#include <stddef.h>
#include <stdlib.h>
#include <exception>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include "PS2OS.h"
#include "Ps2Const.h"
#include "StdStream.h"
#include "PtrMacro.h"
#include "Utils.h"
#include "DMAC.h"
#include "INTC.h"
#include "SIF.h"
#include "COP_SCU.h"
#include "uint128.h"
#include "MIPSAssembler.h"
#include "Profiler.h"
#include "OsEventManager.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/FilteringNodeIterator.h"
#include "Log.h"
#include "iop/IopBios.h"
#ifdef MACOSX
#include "CoreFoundation/CoreFoundation.h"
#endif

// PS2OS Memory Allocation
// Start        End             Description
// 0x80000000	0x80000004		Current Thread ID
// 0x80008000	0x8000A000		DECI2 Handlers
// 0x8000A000	0x8000C000		INTC Handlers
// 0x8000C000	0x8000E000		DMAC Handlers
// 0x8000E000	0x80010000		Semaphores
// 0x80010000   0x80010800      Custom System Call addresses (0x200 entries)
// 0x80011000	0x80020000		Threads
// 0x80020000	0x80030000		Kernel Stack
// 0x80030000	0x80032000		Thread Linked List

// BIOS area
// Start        End             Description
// 0x1FC00004   0x1FC00008      REEXCEPT instruction (for exception reentry) to be changed
// 0x1FC00100	0x1FC00200		Custom System Call handling code
// 0x1FC00200	0x1FC01000		Interrupt Handler
// 0x1FC01000	0x1FC02000		DMAC Interrupt Handler
// 0x1FC02000	0x1FC03000		GS Interrupt Handler
// 0x1FC03000	0x1FC03100		Thread epilogue
// 0x1FC03100	0x1FC03200		Wait Thread Proc

#define BIOS_ADDRESS_BASE			0x1FC00000
#define BIOS_ADDRESS_WAITTHREADPROC	0x1FC03100

#define CONFIGPATH "./config/"
#define PATCHESPATH "patches.xml"

#define THREAD_INIT_QUOTA			(15)

using namespace Framework;
using namespace std;
using namespace boost;

CPS2OS::CPS2OS(CMIPS& ee, CMIPS& vu1, uint8* ram, uint8* bios, CGSHandler*& gs, CSIF& sif, CIopBios& iopBios) :
m_ee(ee),
m_vu1(vu1),
m_gs(gs),
m_pELF(NULL),
m_ram(ram),
m_bios(bios),
m_pThreadSchedule(NULL),
m_sif(sif),
m_iopBios(iopBios)
{
	Initialize();
}

CPS2OS::~CPS2OS()
{
	Release();
}

void CPS2OS::Initialize()
{
	m_pELF = NULL;
//	m_pCtx = &CPS2VM::m_EE;

	m_ee.m_State.nGPR[CMIPS::K0].nV[0] = 0x80030000;
	m_ee.m_State.nGPR[CMIPS::K0].nV[1] = 0xFFFFFFFF;

	m_pThreadSchedule = new CRoundRibbon(m_ram + 0x30000, 0x2000);
}

void CPS2OS::Release()
{
	UnloadExecutable();
	
	DELETEPTR(m_pThreadSchedule);
}

void CPS2OS::DumpThreadSchedule()
{
	THREAD* pThread;
	CRoundRibbon::ITERATOR itThread(m_pThreadSchedule);
	const char* sStatus;

	printf("Thread Schedule Information\r\n");
	printf("---------------------------\r\n");

	for(itThread = m_pThreadSchedule->Begin(); !itThread.IsEnd(); itThread++)
	{
        pThread = GetThread(itThread.GetValue());

        THREADCONTEXT* pContext(reinterpret_cast<THREADCONTEXT*>(&m_ram[pThread->nContextPtr]));

        switch(pThread->nStatus)
		{
		case THREAD_RUNNING:
			sStatus = "Running";
			break;
		case THREAD_SUSPENDED:
			sStatus = "Suspended/Sleeping";
			break;
		case THREAD_WAITING:
			sStatus = "Waiting";
			break;
		case THREAD_ZOMBIE:
			sStatus = "Zombie";
			break;
		default:
			sStatus = "Unknown";
			break;
		}

        printf("Status: %19s, ID: %0.2i, Priority: %0.2i, EPC: 0x%0.8X, RA: 0x%0.8X, WaitSema: %i.\r\n",
            sStatus,
            itThread.GetValue(),
            pThread->nPriority,
            pThread->nEPC,
            pContext->nGPR[CMIPS::RA].nV[0],
            pThread->nSemaWait);
    }
}

void CPS2OS::DumpIntcHandlers()
{
	INTCHANDLER* pHandler;

	printf("INTC Handlers Information\r\n");
	printf("-------------------------\r\n");

	for(unsigned int i = 0; i < MAX_INTCHANDLER; i++)
	{
		pHandler = GetIntcHandler(i + 1);
		if(pHandler->nValid == 0) continue;

		printf("ID: %0.2i, Line: %i, Address: 0x%0.8X.\r\n", \
			i + 1,
			pHandler->nCause,
			pHandler->nAddress);
	}
}

void CPS2OS::DumpDmacHandlers()
{
	DMACHANDLER* pHandler;

	printf("DMAC Handlers Information\r\n");
	printf("-------------------------\r\n");

	for(unsigned int i = 0; i < MAX_DMACHANDLER; i++)
	{
		pHandler = GetDmacHandler(i + 1);
		if(pHandler->nValid == 0) continue;

		printf("ID: %0.2i, Channel: %i, Address: 0x%0.8X.\r\n", \
			i + 1,
			pHandler->nChannel,
			pHandler->nAddress);
	}
}

void CPS2OS::BootFromFile(const char* sPath)
{
	filesystem::path ExecPath(sPath, filesystem::native);
    CStdStream stream(fopen(ExecPath.string().c_str(), "rb"));
	LoadELF(stream, ExecPath.leaf().c_str());
}

void CPS2OS::BootFromCDROM()
{
    string executablePath;
    Iop::CIoman* ioman = m_iopBios.GetIoman();

    {
        uint32 handle = ioman->Open(Iop::Ioman::CDevice::O_RDONLY, "cdrom0:SYSTEM.CNF");
        if(static_cast<int32>(handle) < 0)
        {
		    throw runtime_error("No 'SYSTEM.CNF' file found on the cdrom0 device.");
        }

        {
            CStream* file(ioman->GetFileStream(handle));
        	string line;

	        Utils::GetLine(file, &line);
	        while(!file->IsEOF())
	        {
		        if(!strncmp(line.c_str(), "BOOT2", 5))
		        {
                    const char* tempPath = strstr(line.c_str(), "=");
			        if(tempPath != NULL)
			        {
				        tempPath++;
				        if(tempPath[0] == ' ') tempPath++;
                        executablePath = tempPath;
				        break;
			        }
		        }
		        Utils::GetLine(file, &line);
	        }
        }

        ioman->Close(handle);
    }

	if(executablePath.length() == 0)
	{
		throw runtime_error("Error parsing 'SYSTEM.CNF' for a BOOT2 value.");
	}

    {
	    uint32 handle = ioman->Open(Iop::Ioman::CDevice::O_RDONLY, executablePath.c_str());
        if(static_cast<int32>(handle) < 0)
        {
		    throw runtime_error("Couldn't open executable specified in SYSTEM.CNF.");
        }

        try
        {
            const char* executableName = strchr(executablePath.c_str(), ':') + 1;
            if(executableName[0] == '/' || executableName[0] == '\\') executableName++;
            CStream* file(ioman->GetFileStream(handle));
            LoadELF(*file, executableName);
        }
        catch(...)
        {

        }
        ioman->Close(handle);
    }
}

CELF* CPS2OS::GetELF()
{
	return m_pELF;
}

const char* CPS2OS::GetExecutableName()
{
	return m_sExecutableName.c_str();
}

void CPS2OS::LoadELF(CStream& stream, const char* sExecName)
{
	CELF* pELF;

	try
	{
		pELF = new CELF(&stream);
	}
	catch(const exception& Exception)
	{
		throw Exception;
	}

	//Check for MIPS CPU
	if(pELF->m_Header.nCPU != 8)
	{
		DELETEPTR(pELF);
		throw runtime_error("Invalid target CPU. Must be MIPS.");
	}

	if(pELF->m_Header.nType != 2)
	{
		DELETEPTR(pELF);
		throw runtime_error("Not an executable ELF file.");
	}
	
//	CPS2VM::Pause();

	UnloadExecutable();

	m_pELF = pELF;

	m_sExecutableName = sExecName;

	LoadExecutable();

	//Just a test
//	pStream = new CStdStream(fopen("./vfs/host/sjpcm.irx", "rb"));
//	pStream = new CStdStream(fopen("./vfs/host/padman.irx", "rb"));
//	pELF = new CELF(pStream);
//	memcpy(m_ram + 0x01000000, pELF->m_pData, pELF->m_nLenght);
//	delete pELF;
/*
	int i;
	uint32 nVal;
	for(i = 0; i < 0x02000000 / 4; i++)
	{
		nVal = ((uint32*)m_ram)[i];
		if((nVal & 0xFFFF) == 0x95B0)
		{
			//if((nVal & 0xFC000000) != 0x0C000000)
			{
				printf("Allo: 0x%0.8X\r\n", i * 4);
			}
		}
	}
*/
/*
	int i;
	uint32 nVal;
	for(i = 0; i < 0x02000000 / 4; i++)
	{
		nVal = ((uint32*)m_ram)[i];
		if(nVal == 0x2F9B50)
		{
			printf("Allo: 0x%0.8X\r\n", i * 4);
		}
		if((nVal & 0xFC000000) == 0x0C000000)
		{
			nVal &= 0x3FFFFFF;
			nVal *= 4;
			if(nVal == 0x109D38)
			{
				printf("Allo: 0x%0.8X\r\n", i * 4);
			}
		}
	}
*/
//	*((uint32*)&m_ram[0x0010B5E4]) = 0;
//	*((uint32*)&m_ram[0x002F6C58]) = 0;
//	*((uint32*)&m_ram[0x00109208]) = 0x28840100;
//	*((uint32*)&m_ram[0x0010922C]) = 0;
//	*((uint32*)&m_ram[0x001067C4]) = 0;
//	*((uint32*)&m_ram[0x001AC028]) = 0;

//	*(uint32*)&m_ram[0x0029B758] = 0;
//	*(uint32*)&m_ram[0x0029B768] = 0;
//	*(uint32*)&m_ram[0x0029B774] = 0;

    //REMOVE
    //*reinterpret_cast<uint32*>(&m_ram[m_ee.m_State.nPC + 0x00]) = 0x30C600FF;
    //*reinterpret_cast<uint32*>(&m_ram[m_ee.m_State.nPC + 0x04]) = 0x00C03027;
    //------

	ApplyPatches();

	m_OnExecutableChange();
	COsEventManager::GetInstance().Begin(m_sExecutableName.c_str());

	printf("PS2OS: Loaded '%s' executable file.\r\n", sExecName);
}

void CPS2OS::LoadExecutable()
{
	ELFPROGRAMHEADER* p;
	unsigned int i;
	uint32 nMinAddr;

	nMinAddr = 0xFFFFFFF0;

	for(i = 0; i < m_pELF->m_Header.nProgHeaderCount; i++)
	{
		p = m_pELF->GetProgram(i);
		if(p != NULL)
		{
			if(p->nVAddress < nMinAddr)
			{
				nMinAddr = p->nVAddress;
			}
			memcpy(m_ram + p->nVAddress, (uint8*)m_pELF->m_pData + p->nOffset, p->nFileSize);
		}
	}

	//Load the comments maybe?
	LoadExecutableConfig();

	//InsertFunctionSymbols();

	m_ee.m_State.nPC = m_pELF->m_Header.nEntryPoint;
	
	*(uint32*)&m_bios[0x00000004] = 0x0000001D;
	AssembleCustomSyscallHandler();
	AssembleInterruptHandler();
	AssembleDmacHandler();
	AssembleIntcHandler();
	AssembleThreadEpilog();
	AssembleWaitThreadProc();
	CreateWaitThread();

#ifdef DEBUGGER_INCLUDED
	m_ee.m_pAnalysis->Clear();
	m_ee.m_pAnalysis->Analyse(nMinAddr, (nMinAddr + m_pELF->m_nLenght) & ~0x3);
#endif

//	CPS2VM::m_OnMachineStateChange();
}

void CPS2OS::UnloadExecutable()
{
	m_OnExecutableUnloading();
	COsEventManager::GetInstance().Flush();

	if(m_pELF == NULL) return;

	DELETEPTR(m_pELF);

	SaveExecutableConfig();

	m_ee.m_Comments.RemoveTags();
	m_ee.m_Functions.RemoveTags();
}

void CPS2OS::LoadExecutableConfig()
{

#ifdef DEBUGGER_INCLUDED

	string sPath;

	//Functions
	sPath = CONFIGPATH + m_sExecutableName + ".functions";
	m_ee.m_Functions.Unserialize(sPath.c_str());

	//Comments
	sPath = CONFIGPATH + m_sExecutableName + ".comments";
	m_ee.m_Comments.Unserialize(sPath.c_str());

	//VU1 Comments
	sPath = CONFIGPATH + m_sExecutableName + ".vu1comments";
	m_vu1.m_Comments.Unserialize(sPath.c_str());

#endif

}

void CPS2OS::SaveExecutableConfig()
{

#ifdef DEBUGGER_INCLUDED

	string sPath;

	//Functions
	sPath = CONFIGPATH + m_sExecutableName + ".functions";
	m_ee.m_Functions.Serialize(sPath.c_str());

	//Comments
	sPath = CONFIGPATH + m_sExecutableName + ".comments";
	m_ee.m_Comments.Serialize(sPath.c_str());

	//VU1 Comments
	sPath = CONFIGPATH + m_sExecutableName + ".vu1comments";
	m_vu1.m_Comments.Serialize(sPath.c_str());

#endif

}

void CPS2OS::ApplyPatches()
{
	Xml::CNode* pDocument;
	Xml::CNode* pPatches;

	string patchesPath = PATCHESPATH;
	
#ifdef MACOSX
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyResourceURL(bundle, CFSTR("patches"), CFSTR("xml"), NULL);
	if(url != NULL)
	{
		CFStringRef pathString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		const char* pathCString = CFStringGetCStringPtr(pathString, kCFStringEncodingMacRoman);
		if(pathCString != NULL) 
		{
			patchesPath = pathCString;		
		}
	}
#endif

	try
	{
		CStdStream patchesStream(fopen(patchesPath.c_str(), "rb"));
		pDocument = Xml::CParser::ParseDocument(&patchesStream);
		if(pDocument == NULL) return;
	}
	catch(...)
	{
		return;
	}

	pPatches = pDocument->Select("Patches");
	if(pPatches == NULL)
	{
		delete pDocument;
		return;
	}

	for(Xml::CFilteringNodeIterator itNode(pPatches, "Executable"); !itNode.IsEnd(); itNode++)
	{
		Xml::CNode* pExecutable;
		const char* sName;

		pExecutable = (*itNode);

		sName = pExecutable->GetAttribute("Name");
		if(sName == NULL) continue;

		if(!strcmp(sName, GetExecutableName()))
		{
			//Found the right executable
			unsigned int nPatchCount;

			nPatchCount = 0;

			for(Xml::CFilteringNodeIterator itNode(pExecutable, "Patch"); !itNode.IsEnd(); itNode++)
			{
				Xml::CNode* pPatch;
				const char* sAddress;
				const char* sValue;
				uint32 nValue, nAddress;

				pPatch = (*itNode);
				
				sAddress	= pPatch->GetAttribute("Address");
				sValue		= pPatch->GetAttribute("Value");

				if(sAddress == NULL) continue;
				if(sValue == NULL) continue;

				if(sscanf(sAddress, "%x", &nAddress) == 0) continue;
				if(sscanf(sValue, "%x", &nValue) == 0) continue;

				*(uint32*)&m_ram[nAddress] = nValue;

				nPatchCount++;
			}

			printf("PS2OS: Applied %i patch(es).\r\n", nPatchCount);

			break;
		}
	}

	delete pDocument;
}

void CPS2OS::AssembleCustomSyscallHandler()
{
	CMIPSAssembler Asm((uint32*)&m_bios[0x100]);

	//Epilogue
	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFF0);
	Asm.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	
	//Load the function address off the table at 0x80010000
	Asm.SLL(CMIPS::T0, CMIPS::V1, 2);
	Asm.LUI(CMIPS::T1, 0x8001);
	Asm.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);
	Asm.LW(CMIPS::T0, 0x0000, CMIPS::T0);
	
	//And the address with 0x1FFFFFFF
	Asm.LUI(CMIPS::T1, 0x1FFF);
	Asm.ORI(CMIPS::T1, CMIPS::T1, 0xFFFF);
	Asm.AND(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Jump to the system call address
	Asm.JALR(CMIPS::T0);
	Asm.NOP();

	//Prologue
	Asm.LD(CMIPS::RA, 0x0000, CMIPS::SP);
	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0x0010);
	Asm.ERET();
}

void CPS2OS::AssembleInterruptHandler()
{
	CMIPSAssembler Asm((uint32*)&m_bios[0x200]);
	unsigned int i;

	//Epilogue (allocate 0x204 bytes)
	Asm.ADDIU(CMIPS::K0, CMIPS::K0, 0xFDFC);
	
	//Save context
	for(i = 0; i < 32; i++)
	{
		Asm.SQ(i, (i * 0x10), CMIPS::K0);
	}
	
	//Save EPC
	Asm.MFC0(CMIPS::T0, CCOP_SCU::EPC);
	Asm.SW(CMIPS::T0, 0x0200, CMIPS::K0);

	//Set SP
	Asm.ADDU(CMIPS::SP, CMIPS::K0, CMIPS::R0);

	//Get INTC status
	Asm.LUI(CMIPS::T0, 0x1000);
	Asm.ORI(CMIPS::T0, CMIPS::T0, 0xF000);
	Asm.LW(CMIPS::S0, 0x0000, CMIPS::T0);

	//Get INTC mask
	Asm.LUI(CMIPS::T1, 0x1000);
	Asm.ORI(CMIPS::T1, CMIPS::T1, 0xF010);
	Asm.LW(CMIPS::S1, 0x0000, CMIPS::T1);

	//Get cause
	Asm.AND(CMIPS::S0, CMIPS::S0, CMIPS::S1);

	//Clear cause
	//Asm.SW(CMIPS::S0, 0x0000, CMIPS::T0);
	Asm.NOP();

	//Check if INT1 (DMAC)
	Asm.ANDI(CMIPS::T0, CMIPS::S0, 0x0002);
	Asm.BEQ(CMIPS::R0, CMIPS::T0, 0x0005);
	Asm.NOP();

	//Go to DMAC interrupt handler
	Asm.LUI(CMIPS::T0, 0x1FC0);
	Asm.ORI(CMIPS::T0, CMIPS::T0, 0x1000);
	Asm.JALR(CMIPS::T0);
	Asm.NOP();

	//Check if INT2 (Vblank Start)
	Asm.ANDI(CMIPS::T0, CMIPS::S0, 0x0004);
	Asm.BEQ(CMIPS::R0, CMIPS::T0, 0x0006);
	Asm.NOP();

	//Process handlers
	Asm.LUI(CMIPS::T0, 0x1FC0);
	Asm.ORI(CMIPS::T0, CMIPS::T0, 0x2000);
	Asm.ADDIU(CMIPS::A0, CMIPS::R0, 0x0002);
	Asm.JALR(CMIPS::T0);
	Asm.NOP();

	//Check if INT3 (Vblank End)
	Asm.ANDI(CMIPS::T0, CMIPS::S0, 0x0008);
	Asm.BEQ(CMIPS::R0, CMIPS::T0, 0x0006);
	Asm.NOP();

	//Process handlers
	Asm.LUI(CMIPS::T0, 0x1FC0);
	Asm.ORI(CMIPS::T0, CMIPS::T0, 0x2000);
	Asm.ADDIU(CMIPS::A0, CMIPS::R0, 0x0003);
	Asm.JALR(CMIPS::T0);
	Asm.NOP();

	//Check if INT11 (Timer2)
	Asm.ANDI(CMIPS::T0, CMIPS::S0, 0x0800);
	Asm.BEQ(CMIPS::R0, CMIPS::T0, 0x0006);
	Asm.NOP();

	//Process handlers
	Asm.LUI(CMIPS::T0, 0x1FC0);
	Asm.ORI(CMIPS::T0, CMIPS::T0, 0x2000);
	Asm.ADDIU(CMIPS::A0, CMIPS::R0, 0x000B);
	Asm.JALR(CMIPS::T0);
	Asm.NOP();

	//Restore EPC
	Asm.LW(CMIPS::T0, 0x0200, CMIPS::K0);
	Asm.MTC0(CMIPS::T0, CCOP_SCU::EPC);

	//Restore Context
	for(i = 0; i < 32; i++)
	{
		Asm.LQ(i, (i * 0x10), CMIPS::K0);
	}

	//Prologue
	Asm.ADDIU(CMIPS::K0, CMIPS::K0, 0x204);
	Asm.ERET();
}

void CPS2OS::AssembleDmacHandler()
{
	CMIPSAssembler Asm((uint32*)&m_bios[0x1000]);

	//Prologue
	//S0 -> Channel Counter
	//S1 -> DMA Interrupt Status
	//S2 -> Handler Counter

	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE0);
	Asm.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	Asm.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	Asm.SD(CMIPS::S1, 0x0010, CMIPS::SP);
	Asm.SD(CMIPS::S2, 0x0018, CMIPS::SP);

    //Clear INTC cause
	Asm.LUI(CMIPS::T1, 0x1000);
	Asm.ORI(CMIPS::T1, CMIPS::T1, 0xF000);
    Asm.ADDIU(CMIPS::T0, CMIPS::R0, 0x0002);
	Asm.SW(CMIPS::T0, 0x0000, CMIPS::T1);

	//Load the DMA interrupt status
	Asm.LUI(CMIPS::T0, 0x1000);
	Asm.ORI(CMIPS::T0, CMIPS::T0, 0xE010);
	Asm.LW(CMIPS::T0, 0x0000, CMIPS::T0);

	Asm.SRL(CMIPS::T1, CMIPS::T0, 16);
	Asm.AND(CMIPS::S1, CMIPS::T0, CMIPS::T1);

	//Initialize channel counter
	Asm.ADDIU(CMIPS::S0, CMIPS::R0, 0x0009);

	//Check if that specific DMA channel interrupt is the cause
	Asm.ORI(CMIPS::T0, CMIPS::R0, 0x0001);
	Asm.SLLV(CMIPS::T0, CMIPS::T0, CMIPS::S0);
	Asm.AND(CMIPS::T0, CMIPS::T0, CMIPS::S1);
	Asm.BEQ(CMIPS::T0, CMIPS::R0, 0x001A);
	Asm.NOP();

	//Clear interrupt
	Asm.LUI(CMIPS::T1, 0x1000);
	Asm.ORI(CMIPS::T1, CMIPS::T1, 0xE010);
	Asm.SW(CMIPS::T0, 0x0000, CMIPS::T1);

	//Initialize DMAC handler loop
	Asm.ADDU(CMIPS::S2, CMIPS::R0, CMIPS::R0);

	//Get the address to the current DMACHANDLER structure
	Asm.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(DMACHANDLER));
	Asm.MULTU(CMIPS::T0, CMIPS::S2, CMIPS::T0);
	Asm.LUI(CMIPS::T1, 0x8000);
	Asm.ORI(CMIPS::T1, CMIPS::T1, 0xC000);
	Asm.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Check validity
	Asm.LW(CMIPS::T1, 0x0000, CMIPS::T0);
	Asm.BEQ(CMIPS::T1, CMIPS::R0, 0x000A);
	Asm.NOP();

	//Check if the channel is good one
	Asm.LW(CMIPS::T1, 0x0004, CMIPS::T0);
	Asm.BNE(CMIPS::S0, CMIPS::T1, 0x0007);
	Asm.NOP();

	//Load the necessary stuff
	Asm.LW(CMIPS::T1, 0x0008, CMIPS::T0);
	Asm.ADDU(CMIPS::A0, CMIPS::S0, CMIPS::R0);
	Asm.LW(CMIPS::A1, 0x000C, CMIPS::T0);
	Asm.LW(CMIPS::GP, 0x0010, CMIPS::T0);
	
	//Jump
	Asm.JALR(CMIPS::T1);
	Asm.NOP();

	//Increment handler counter and test
	Asm.ADDIU(CMIPS::S2, CMIPS::S2, 0x0001);
	Asm.ADDIU(CMIPS::T0, CMIPS::R0, MAX_DMACHANDLER - 1);
	Asm.BNE(CMIPS::S2, CMIPS::T0, 0xFFEC);
	Asm.NOP();

	//Decrement channel counter and test
	Asm.ADDIU(CMIPS::S0, CMIPS::S0, 0xFFFF);
	Asm.BGEZ(CMIPS::S0, 0xFFE0);
	Asm.NOP();

	//Epilogue
	Asm.LD(CMIPS::RA, 0x0000, CMIPS::SP);
	Asm.LD(CMIPS::S0, 0x0008, CMIPS::SP);
	Asm.LD(CMIPS::S1, 0x0010, CMIPS::SP);
	Asm.LD(CMIPS::S2, 0x0018, CMIPS::SP);
	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0x20);
	Asm.JR(CMIPS::RA);
	Asm.NOP();
}

void CPS2OS::AssembleIntcHandler()
{
	CMIPSAssembler Asm((uint32*)&m_bios[0x2000]);

	//Prologue
	//S0 -> Handler Counter

	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE8);
	Asm.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	Asm.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	Asm.SD(CMIPS::S1, 0x0010, CMIPS::SP);

    //Clear INTC cause
	Asm.LUI(CMIPS::T1, 0x1000);
	Asm.ORI(CMIPS::T1, CMIPS::T1, 0xF000);
    Asm.ADDIU(CMIPS::T0, CMIPS::R0, 0x0001);
    Asm.SLLV(CMIPS::T0, CMIPS::T0, CMIPS::A0);
	Asm.SW(CMIPS::T0, 0x0000, CMIPS::T1);

    //Initialize INTC handler loop
	Asm.ADDU(CMIPS::S0, CMIPS::R0, CMIPS::R0);
	Asm.ADDU(CMIPS::S1, CMIPS::A0, CMIPS::R0);

	//Get the address to the current INTCHANDLER structure
	Asm.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(INTCHANDLER));
	Asm.MULTU(CMIPS::T0, CMIPS::S0, CMIPS::T0);
	Asm.LUI(CMIPS::T1, 0x8000);
	Asm.ORI(CMIPS::T1, CMIPS::T1, 0xA000);
	Asm.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Check validity
	Asm.LW(CMIPS::T1, 0x0000, CMIPS::T0);
	Asm.BEQ(CMIPS::T1, CMIPS::R0, 0x0009);
	Asm.NOP();

	//Check if the cause is good one
	Asm.LW(CMIPS::T1, 0x0004, CMIPS::T0);
	Asm.BNE(CMIPS::S1, CMIPS::T1, 0x0006);
	Asm.NOP();

	//Load the necessary stuff
	Asm.LW(CMIPS::T1, 0x0008, CMIPS::T0);
	Asm.LW(CMIPS::A0, 0x000C, CMIPS::T0);
	Asm.LW(CMIPS::GP, 0x0010, CMIPS::T0);
	
	//Jump
	Asm.JALR(CMIPS::T1);
	Asm.NOP();

	//Increment handler counter and test
	Asm.ADDIU(CMIPS::S0, CMIPS::S0, 0x0001);
	Asm.ADDIU(CMIPS::T0, CMIPS::R0, MAX_INTCHANDLER - 1);
	Asm.BNE(CMIPS::S0, CMIPS::T0, 0xFFED);
	Asm.NOP();

	//Epilogue
	Asm.LD(CMIPS::RA, 0x0000, CMIPS::SP);
	Asm.LD(CMIPS::S0, 0x0008, CMIPS::SP);
	Asm.LD(CMIPS::S1, 0x0010, CMIPS::SP);
	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0x18);
	Asm.JR(CMIPS::RA);
	Asm.NOP();
}

void CPS2OS::AssembleThreadEpilog()
{
	CMIPSAssembler Asm((uint32*)&m_bios[0x3000]);
	
	Asm.ADDIU(CMIPS::V1, CMIPS::R0, 0x23);
	Asm.SYSCALL();
}

void CPS2OS::AssembleWaitThreadProc()
{
	CMIPSAssembler Asm((uint32*)&m_bios[BIOS_ADDRESS_WAITTHREADPROC - BIOS_ADDRESS_BASE]);

//	Asm.ADDIU(CMIPS::V1, CMIPS::R0, 0x03);
//	Asm.SYSCALL();

	Asm.BEQ(CMIPS::R0, CMIPS::R0, 0xFFFF);
	Asm.NOP();
}

uint32* CPS2OS::GetCustomSyscallTable()
{
	return (uint32*)&m_ram[0x00010000];
}

uint32 CPS2OS::GetCurrentThreadId()
{
	return *(uint32*)&m_ram[0x00000000];
}

void CPS2OS::SetCurrentThreadId(uint32 nThread)
{
	*(uint32*)&m_ram[0x00000000] = nThread;
}

uint32 CPS2OS::GetNextAvailableThreadId()
{
	uint32 i;
	THREAD* pThread;

	for(i = 0; i < MAX_THREAD; i++)
	{
		pThread = GetThread(i);
		if(pThread->nValid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::THREAD* CPS2OS::GetThread(uint32 nID)
{
	return &((THREAD*)&m_ram[0x00011000])[nID];
}

void CPS2OS::ThreadShakeAndBake()
{
	THREAD* pThread;
	unsigned int nId;

	//Don't play with fire (don't switch if we're in exception mode)
	if(m_ee.m_State.nCOP0[CCOP_SCU::STATUS] & 0x02)
	{
		return;
	}

	if(GetCurrentThreadId() == 0)
	{
		return;
	}

	CRoundRibbon::ITERATOR itThread(m_pThreadSchedule);

	//First of all, revoke the current's thread right to execute itself
	pThread = GetThread(GetCurrentThreadId());
	pThread->nQuota--;

	//Check if all quotas expired
	if(ThreadHasAllQuotasExpired())
	{
		//If so, regive a quota to everyone
		for(itThread = m_pThreadSchedule->Begin(); !itThread.IsEnd(); itThread++)
		{
			nId = itThread.GetValue();
			pThread = GetThread(nId);

			pThread->nQuota = THREAD_INIT_QUOTA;
		}
	}

	//Next, find the next suitable thread to execute
	for(itThread = m_pThreadSchedule->Begin(); !itThread.IsEnd(); itThread++)
	{
		nId = itThread.GetValue();
		pThread = GetThread(nId);

		if(pThread->nStatus != THREAD_RUNNING) continue;
//		if(pThread->nQuota == 0) continue;
		break;
	}


	if(itThread.IsEnd())
	{
		//Deadlock or something here
		assert(0);
		nId = 0;
	}
	else
	{
		//Remove and readd the thread into the queue
		m_pThreadSchedule->Remove(pThread->nScheduleID);
		pThread->nScheduleID = m_pThreadSchedule->Insert(nId, pThread->nPriority);
	}

	ThreadSwitchContext(nId);
}

bool CPS2OS::ThreadHasAllQuotasExpired()
{
	CRoundRibbon::ITERATOR itThread(m_pThreadSchedule);

	for(itThread = m_pThreadSchedule->Begin(); !itThread.IsEnd(); itThread++)
	{
		THREAD* pThread;
		unsigned int nId;

		nId = itThread.GetValue();
		pThread = GetThread(nId);

		if(pThread->nStatus != THREAD_RUNNING) continue;
		if(pThread->nQuota == 0) continue;

		return false;
	}

	return true;
}

void CPS2OS::ThreadSwitchContext(unsigned int nID)
{
	//Save the context of the current thread
	THREAD* pThread;
	THREADCONTEXT* pContext;

	if(nID == GetCurrentThreadId()) return;

	pThread = GetThread(GetCurrentThreadId());
	pContext = (THREADCONTEXT*)&m_ram[pThread->nContextPtr];

	//Save the context
	for(uint32 i = 0; i < 0x20; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		pContext->nGPR[i] = m_ee.m_State.nGPR[i];
	}

	pThread->nEPC = m_ee.m_State.nPC;

	SetCurrentThreadId(nID);
	pThread = GetThread(GetCurrentThreadId());
	pContext = (THREADCONTEXT*)&m_ram[pThread->nContextPtr];

	//Load the context

	m_ee.m_State.nPC = pThread->nEPC;

	for(uint32 i = 0; i < 0x20; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		m_ee.m_State.nGPR[i] = pContext->nGPR[i];
	}

//	if(CPS2VM::m_Logging.GetOSLoggingStatus())
//	{
//		printf("PS2OS: New thread elected (id = %i).\r\n", nID);
//	}
    CLog::GetInstance().Print("ps2os", "New thread elected (id = %i).\r\n", nID);
}

/*
uint32 CPS2OS::GetNextReadyThread()
{
	CRoundRibbon::ITERATOR itThread(m_pThreadSchedule);
	THREAD* pThread;
	unsigned int nID;

//	unsigned int nRand, nCount;
//	srand((unsigned int)time(NULL));
//	nRand = rand();

//	nCount = 0;
//	for(unsigned int i = 1; i < MAX_THREAD; i++)
//	{
//		if(i == GetCurrentThreadId()) continue;
//		pThread = GetThread(i);
//		if(pThread->nValid != 1) continue;
//		if(pThread->nStatus != THREAD_RUNNING) continue;
//		nCount++;
//	}
//
//
//	if(nCount == 0)
//	{
//		nID = GetCurrentThreadId();
//
//		pThread = GetThread(nID);
//		if(pThread->nStatus != THREAD_RUNNING)
//		{
//			//Now, now, everyone is waiting for something...
//			nID = 0;
//		}
//
//		return nID;
//	}
//
//	nRand %= nCount;
//
//	nCount = 0;
//	for(unsigned int i = 1; i < MAX_THREAD; i++)
//	{
//		if(i == GetCurrentThreadId()) continue;
//		pThread = GetThread(i);
//		if(pThread->nValid != 1) continue;
//		if(pThread->nStatus != THREAD_RUNNING) continue;
//		if(nRand == nCount)
//		{
//			nID = i;
//			break;
//		}
//		nCount++;
//	}
//
//	return nID;

	for(itThread = m_pThreadSchedule->Begin(); !itThread.IsEnd(); itThread++)
	{
		nID = itThread.GetValue();
		pThread = GetThread(nID);
		if(pThread->nStatus != THREAD_RUNNING) continue;
		break;
	}


	if(itThread.IsEnd())
	{
		//Deadlock or something here
		assert(0);
	}

	//Insert and readd the thread
	m_pThreadSchedule->Remove(pThread->nScheduleID);
	m_pThreadSchedule->Insert(nID, pThread->nPriority);

	return nID;

}
*/

void CPS2OS::CreateWaitThread()
{
	THREAD* pThread;

	pThread = GetThread(0);
	pThread->nValid		= 1;
	pThread->nEPC		= BIOS_ADDRESS_WAITTHREADPROC;
	pThread->nStatus	= THREAD_ZOMBIE;
}

uint32 CPS2OS::GetNextAvailableSemaphoreId()
{
	uint32 i;
	SEMAPHORE* pSemaphore;

	for(i = 1; i < MAX_SEMAPHORE; i++)
	{
		pSemaphore = GetSemaphore(i);
		if(pSemaphore->nValid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::SEMAPHORE* CPS2OS::GetSemaphore(uint32 nID)
{
	nID--;
	return &((SEMAPHORE*)&m_ram[0x0000E000])[nID];
}

uint32 CPS2OS::GetNextAvailableDmacHandlerId()
{
	uint32 i;
	DMACHANDLER* pHandler;

	for(i = 1; i < MAX_DMACHANDLER; i++)
	{
		pHandler = GetDmacHandler(i);
		if(pHandler->nValid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::DMACHANDLER* CPS2OS::GetDmacHandler(uint32 nID)
{
	nID--;
	return &((DMACHANDLER*)&m_ram[0x0000C000])[nID];
}

uint32 CPS2OS::GetNextAvailableIntcHandlerId()
{
	uint32 i;
	INTCHANDLER* pHandler;

	for(i = 1; i < MAX_INTCHANDLER; i++)
	{
		pHandler = GetIntcHandler(i);
		if(pHandler->nValid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::INTCHANDLER* CPS2OS::GetIntcHandler(uint32 nID)
{
	nID--;
	return &((INTCHANDLER*)&m_ram[0x0000A000])[nID];
}

uint32 CPS2OS::GetNextAvailableDeci2HandlerId()
{
	uint32 i;
	DECI2HANDLER* pHandler;

	for(i = 1; i < MAX_DECI2HANDLER; i++)
	{
		pHandler = GetDeci2Handler(i);
		if(pHandler->nValid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::DECI2HANDLER* CPS2OS::GetDeci2Handler(uint32 nID)
{
	nID--;
	return &((DECI2HANDLER*)&m_ram[0x00008000])[nID];
}

void CPS2OS::ExceptionHandler()
{
	ThreadShakeAndBake();
	m_ee.GenerateInterrupt(0x1FC00200);
}

uint32 CPS2OS::TranslateAddress(CMIPS* pCtx, uint32 nVAddrHI, uint32 nVAddrLO)
{
	if(nVAddrLO >= 0x70000000 && nVAddrLO <= 0x70003FFF)
	{
		return (nVAddrLO - 0x6E000000);
	}

	return nVAddrLO & 0x1FFFFFFF;
}

//////////////////////////////////////////////////
//System Calls
//////////////////////////////////////////////////

void CPS2OS::sc_Unhandled()
{
	printf("PS2OS: Unknown system call (%X) called from 0x%0.8X.\r\n", m_ee.m_State.nGPR[3].nV[0], m_ee.m_State.nPC);
}

//02
void CPS2OS::sc_GsSetCrt()
{
	unsigned int nMode;
	bool nIsInterlaced;
	bool nIsFrameMode;

	nIsInterlaced	= (m_ee.m_State.nGPR[SC_PARAM0].nV[0] != 0);
	nMode			= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	nIsFrameMode	= (m_ee.m_State.nGPR[SC_PARAM2].nV[0] != 0);

	if(m_gs != NULL)
	{
		m_gs->SetCrt(nIsInterlaced, nMode, nIsFrameMode);
	}
}

//10
void CPS2OS::sc_AddIntcHandler()
{
	uint32 nAddress, nCause, nNext, nArg, nID;
	INTCHANDLER* pHandler;

	nCause		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nAddress	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	nNext		= m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	nArg		= m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	/*
	if(nNext != 0)
	{
		assert(0);
	}
	*/

	nID = GetNextAvailableIntcHandlerId();
	if(nID == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pHandler = GetIntcHandler(nID);
	pHandler->nValid	= 1;
	pHandler->nAddress	= nAddress;
	pHandler->nCause	= nCause;
	pHandler->nArg		= nArg;
	pHandler->nGP		= m_ee.m_State.nGPR[CMIPS::GP].nV[0];

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//11
void CPS2OS::sc_RemoveIntcHandler()
{
	uint32 nCause, nID;
	INTCHANDLER* pHandler;

	nCause		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nID			= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	pHandler = GetIntcHandler(nID);
	if(pHandler->nValid != 1)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pHandler->nValid = 0;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//12
void CPS2OS::sc_AddDmacHandler()
{
	uint32 nAddress, nChannel, nNext, nArg, nID;
	DMACHANDLER* pHandler;

	nChannel	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nAddress	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	nNext		= m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	nArg		= m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	//The Next parameter indicates at which moment we'd want our DMAC handler to be called.
	//-1 -> At the end
	//0  -> At the start
	//n  -> After handler 'n'

	if(nNext != 0)
	{
		assert(0);
	}

	nID = GetNextAvailableDmacHandlerId();
	if(nID == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pHandler = GetDmacHandler(nID);
	pHandler->nValid	= 1;
	pHandler->nAddress	= nAddress;
	pHandler->nChannel	= nChannel;
	pHandler->nArg		= nArg;
	pHandler->nGP		= m_ee.m_State.nGPR[CMIPS::GP].nV[0];

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//13
void CPS2OS::sc_RemoveDmacHandler()
{
	uint32 nChannel, nID;
	DMACHANDLER* pHandler;

	nChannel	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nID			= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	pHandler = GetDmacHandler(nID);
	pHandler->nValid = 0x00;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//14
void CPS2OS::sc_EnableIntc()
{
	uint32 nCause = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 nMask = 1 << nCause;

	if(!(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & nMask))
	{
		m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, nMask);
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 1;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//15
void CPS2OS::sc_DisableIntc()
{
    uint32 nCause = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
    uint32 nMask = 1 << nCause;
	if(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & nMask)
	{
		m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, nMask);
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 1;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//16
void CPS2OS::sc_EnableDmac()
{
	uint32 nChannel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 nRegister = 0x10000 << nChannel;

	if(!(m_ee.m_pMemoryMap->GetWord(CDMAC::D_STAT) & nRegister))
	{
		m_ee.m_pMemoryMap->SetWord(CDMAC::D_STAT, nRegister);
	}

	//Enable INT1
	if(!(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & 0x02))
	{
		m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, 0x02);
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 1;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//17
void CPS2OS::sc_DisableDmac()
{
    uint32 nChannel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
    uint32 nRegister = 0x10000 << nChannel;

    if(m_ee.m_pMemoryMap->GetWord(CDMAC::D_STAT) & nRegister)
    {
	    m_ee.m_pMemoryMap->SetWord(CDMAC::D_STAT, nRegister);
    }

    m_ee.m_State.nGPR[SC_RETURN].nD0 = 1;
}

//20
void CPS2OS::sc_CreateThread()
{
	THREADPARAM* pThreadParam;
	THREAD* pThread;
	THREADCONTEXT* pContext;
	uint32 nID, nStackAddr, nHeapBase;

	pThreadParam = (THREADPARAM*)&m_ram[m_ee.m_State.nGPR[SC_PARAM0].nV[0]];

	nID = GetNextAvailableThreadId();
	if(nID == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
		return;
	}

	pThread = GetThread(GetCurrentThreadId());
	nHeapBase = pThread->nHeapBase;

	pThread = GetThread(nID);
	pThread->nValid			= 1;
	pThread->nStatus		= THREAD_SUSPENDED;
	pThread->nStackBase		= pThreadParam->nStackBase;
	pThread->nEPC			= pThreadParam->nThreadProc;
	pThread->nPriority		= pThreadParam->nPriority;
	pThread->nHeapBase		= nHeapBase;
	pThread->nWakeUpCount	= 0;
	pThread->nQuota			= THREAD_INIT_QUOTA;
	pThread->nScheduleID	= m_pThreadSchedule->Insert(nID, pThreadParam->nPriority);
	pThread->nStackSize		= pThreadParam->nStackSize;

	nStackAddr = pThreadParam->nStackBase + pThreadParam->nStackSize - STACKRES;
	pThread->nContextPtr	= nStackAddr;

	assert(sizeof(THREADCONTEXT) == STACKRES);

	pContext = (THREADCONTEXT*)&m_ram[pThread->nContextPtr];
	memset(pContext, 0, sizeof(THREADCONTEXT));

	pContext->nGPR[CMIPS::SP].nV0 = nStackAddr;
	pContext->nGPR[CMIPS::FP].nV0 = nStackAddr;
	pContext->nGPR[CMIPS::GP].nV0 = pThreadParam->nGP;
	pContext->nGPR[CMIPS::RA].nV0 = 0x1FC03000;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//21
void CPS2OS::sc_DeleteThread()
{
	uint32 nID;
	THREAD* pThread;

	nID = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	m_pThreadSchedule->Remove(pThread->nScheduleID);

	pThread->nValid = 0;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//22
void CPS2OS::sc_StartThread()
{
	uint32 nID, nArg;
	THREAD* pThread;
	THREADCONTEXT* pContext;

	nID		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nArg	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pThread->nStatus = THREAD_RUNNING;

	pContext = (THREADCONTEXT*)&m_ram[pThread->nContextPtr];
	pContext->nGPR[CMIPS::A0].nV0 = nArg;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//23
void CPS2OS::sc_ExitThread()
{
	THREAD* pThread;

	pThread = GetThread(GetCurrentThreadId());
	pThread->nStatus = THREAD_ZOMBIE;

	ThreadShakeAndBake();
}

//25
void CPS2OS::sc_TerminateThread()
{
	uint32 nID;
	THREAD* pThread;

	nID = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pThread->nStatus = THREAD_ZOMBIE;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//29
void CPS2OS::sc_ChangeThreadPriority()
{
	uint32 nID, nPrio, nPrevPrio;
	THREAD* pThread;

	nID		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nPrio	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	nPrevPrio = pThread->nPriority;
	pThread->nPriority = nPrio;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nPrevPrio;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

	//Reschedule?
	m_pThreadSchedule->Remove(pThread->nScheduleID);
	pThread->nScheduleID = m_pThreadSchedule->Insert(nID, pThread->nPriority);

	ThreadShakeAndBake();
}

//2B
void CPS2OS::sc_RotateThreadReadyQueue()
{
    //TODO: Need to set the thread's new ScheduleId to the return value of ThreadSchedule->Insert
    throw runtime_error("Recheck needed.");

	uint32 nPrio, nID;
	CRoundRibbon::ITERATOR itThread(m_pThreadSchedule);

	nPrio = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	//TODO: Rescheduling isn't always necessary and will cause the current thread's priority queue to be
	//rotated too since each time a thread is picked to be executed it's placed at the end of the queue...

	//Find first of this priority and reinsert if it's the same as the current thread
	//If it's not the same, the schedule will be rotated when another thread is choosen
	for(itThread = m_pThreadSchedule->Begin(); !itThread.IsEnd(); itThread++)
	{
		if(itThread.GetWeight() == nPrio)
		{
			nID = itThread.GetValue();
			if(nID == GetCurrentThreadId())
			{
				m_pThreadSchedule->Remove(itThread.GetIndex());
				m_pThreadSchedule->Insert(nID, nPrio);
			}
			break;
		}
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nPrio;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

	if(!itThread.IsEnd())
	{
		//Change has been made
		ThreadShakeAndBake();
	}
}

//2F
void CPS2OS::sc_GetThreadId()
{
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = GetCurrentThreadId();
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//30
void CPS2OS::sc_ReferThreadStatus()
{
	//THS_RUN = 0x01, THS_READY = 0x02, THS_WAIT = 0x04, THS_SUSPEND = 0x08, THS_DORMANT = 0x10
	uint32 nID, nStatusPtr, nRet;
	THREAD* pThread;

	nID			= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nStatusPtr	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

//	assert(nStatusPtr == 0);

	switch(pThread->nStatus)
	{
	case THREAD_RUNNING:
		nRet = 0x01;
		break;
	case THREAD_WAITING:
		nRet = 0x04;
		break;
	case THREAD_SUSPENDED:
		nRet = 0x10;
		break;
	}

	if(nStatusPtr != 0)
	{
		THREADPARAM* pThreadParam;

		pThreadParam = (THREADPARAM*)(&m_ram[nStatusPtr]);
		pThreadParam->nStatus		= nRet;
		pThreadParam->nPriority		= pThread->nPriority;
		pThreadParam->nStackBase	= pThread->nStackBase;
		pThreadParam->nStackSize	= pThread->nStackSize;
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nRet;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//32
void CPS2OS::sc_SleepThread()
{
	THREAD* pThread;

	pThread = GetThread(GetCurrentThreadId());
	if(pThread->nWakeUpCount == 0)
	{
		pThread->nStatus = THREAD_SUSPENDED;
		ThreadShakeAndBake();
		return;
	}

	pThread->nWakeUpCount--;
}

//33
//34
void CPS2OS::sc_WakeupThread()
{
	uint32 nID		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	bool nInt       = m_ee.m_State.nGPR[3].nV[0] == 0x34;

	THREAD* pThread = GetThread(nID);

	if(pThread->nStatus == THREAD_SUSPENDED)
	{
		pThread->nStatus = THREAD_RUNNING;
		ThreadShakeAndBake();
	}
	else
	{
		pThread->nWakeUpCount++;
	}
}

//3C
void CPS2OS::sc_RFU060()
{
	uint32 nStackBase, nStackSize, nStackAddr;
	THREAD* pThread;

	nStackBase = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	nStackSize = m_ee.m_State.nGPR[SC_PARAM2].nV[0];

	if(nStackBase == 0xFFFFFFFF)
	{
		nStackAddr = 0x02000000;
	}
	else
	{
		nStackAddr = nStackBase + nStackSize;
	}

	//Set up the main thread

	pThread = GetThread(1);
	
	pThread->nValid			= 0x01;
	pThread->nStatus		= THREAD_RUNNING;
	pThread->nStackBase		= nStackAddr - nStackSize;
	pThread->nPriority		= 0;
	pThread->nQuota			= THREAD_INIT_QUOTA;
	pThread->nScheduleID	= m_pThreadSchedule->Insert(1, pThread->nPriority);

	nStackAddr -= STACKRES;
	pThread->nContextPtr	= nStackAddr;

	SetCurrentThreadId(1);

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nStackAddr;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//3D
void CPS2OS::sc_RFU061()
{
	uint32 nHeapBase, nHeapSize;
	THREAD* pThread;

	pThread = GetThread(GetCurrentThreadId());

	nHeapBase = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nHeapSize = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	if(nHeapSize == 0xFFFFFFFF)
	{
		pThread->nHeapBase = pThread->nStackBase;
	}
	else
	{
		pThread->nHeapBase = nHeapBase + nHeapSize;
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = pThread->nHeapBase;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//3E
void CPS2OS::sc_EndOfHeap()
{
	THREAD* pThread;

	pThread = GetThread(GetCurrentThreadId());

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = pThread->nHeapBase;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//40
void CPS2OS::sc_CreateSema()
{
	SEMAPHOREPARAM* pSemaParam;
	SEMAPHORE* pSema;
	uint32 nID;

	pSemaParam = (SEMAPHOREPARAM*)(m_ram + m_ee.m_State.nGPR[SC_PARAM0].nV[0]);

	nID = GetNextAvailableSemaphoreId();
	if(nID == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pSema = GetSemaphore(nID);
	pSema->nValid		= 1;
	pSema->nCount		= pSemaParam->nInitCount;
	pSema->nMaxCount	= pSemaParam->nMaxCount;
	pSema->nWaitCount	= 0;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//41
void CPS2OS::sc_DeleteSema()
{
	uint32 nID;
	SEMAPHORE* pSema;

	nID = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	//Check if any threads are waiting for this?
	if(pSema->nWaitCount != 0)
	{
		assert(0);
	}

	pSema->nValid = 0;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//42
//43
void CPS2OS::sc_SignalSema()
{
	bool nInt;
	uint32 nID, i;
	SEMAPHORE* pSema;
	THREAD* pThread;

	nInt	= m_ee.m_State.nGPR[3].nV[0] == 0x43;
	nID		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}
	
	if(pSema->nWaitCount != 0)
	{
		//Unsleep all threads if they were waiting
		for(i = 0; i < MAX_THREAD; i++)
		{
			pThread = GetThread(i);
			if(!pThread->nValid) continue;
			if(pThread->nStatus != THREAD_WAITING) continue;
			if(pThread->nSemaWait != nID) continue;

			pThread->nStatus	= THREAD_RUNNING;
			pThread->nQuota		= THREAD_INIT_QUOTA;
			pSema->nWaitCount--;

			if(pSema->nWaitCount == 0)
			{
				break;
			}
		}

		m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

		if(!nInt)
		{
			ThreadShakeAndBake();
		}
	}
	else
	{
		pSema->nCount++;
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//44
void CPS2OS::sc_WaitSema()
{
	SEMAPHORE* pSema;
	THREAD* pThread;
	uint32 nID;

	nID = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	if(pSema->nCount == 0)
	{
		//Put this thread in sleep mode and reschedule...
		pSema->nWaitCount++;

		pThread = GetThread(GetCurrentThreadId());
		pThread->nStatus	= THREAD_WAITING;
		pThread->nSemaWait	= nID;

		ThreadShakeAndBake();

		return;
	}

	if(pSema->nCount != 0)
	{
		pSema->nCount--;
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//45
void CPS2OS::sc_PollSema()
{
	uint32 nID;
	SEMAPHORE* pSema;

	nID = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	if(pSema->nCount == 0)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pSema->nCount--;
	
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//48
void CPS2OS::sc_ReferSemaStatus()
{
	uint32 nID;
	SEMAPHORE* pSema;
	SEMAPHOREPARAM *pSemaParam;

	nID = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	pSemaParam = (SEMAPHOREPARAM*)(m_ram + (m_ee.m_State.nGPR[SC_PARAM1].nV[0] & 0x1FFFFFFF));

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pSemaParam->nCount			= pSema->nCount;
	pSemaParam->nMaxCount		= pSema->nMaxCount;
	pSemaParam->nWaitThreads	= pSema->nWaitCount;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//64
void CPS2OS::sc_FlushCache()
{

}

//71
void CPS2OS::sc_GsPutIMR()
{
	uint32 nIMR;

	nIMR = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	if(m_gs != NULL)
	{
		m_gs->WritePrivRegister(CGSHandler::GS_IMR, nIMR);	
	}
}

//73
void CPS2OS::sc_SetVSyncFlag()
{
	uint32 nPtr1, nPtr2;

	nPtr1	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nPtr2	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	*(uint32*)&m_ram[nPtr1] = 0x01;

	if(m_gs != NULL)
	{
		//*(uint32*)&m_ram[nPtr2] = 0x2000;
		*(uint32*)&m_ram[nPtr2] = m_gs->ReadPrivRegister(CGSHandler::GS_CSR) & 0x2000;
	}
	else
	{
		//Humm...
		*(uint32*)&m_ram[nPtr2] = 0;
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//74
void CPS2OS::sc_SetSyscall()
{
	uint32 nAddress;
	uint8 nNumber;

	nNumber		= (uint8)(m_ee.m_State.nGPR[SC_PARAM0].nV[0] & 0xFF);
	nAddress	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	GetCustomSyscallTable()[nNumber]	= nAddress;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//76
void CPS2OS::sc_SifDmaStat()
{
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
}

//77
void CPS2OS::sc_SifSetDma()
{
    struct DMAREG
	{
		uint32 nSrcAddr;
		uint32 nDstAddr;
		uint32 nSize;
		uint32 nFlags;
	};

	DMAREG* pXfer = (DMAREG*)(m_ram + m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
	uint32 nCount = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	//Returns count
	//DMA might call an interrupt handler
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(nCount);

	for(unsigned int i = 0; i < nCount; i++)
	{
		uint32 nSize = (pXfer[i].nSize + 0x0F) / 0x10;

		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_MADR,	pXfer[i].nSrcAddr);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_TADR,	pXfer[i].nDstAddr);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_QWC,	nSize);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_CHCR,	0x00000100);
	}
}

//78
void CPS2OS::sc_SifSetDChain()
{
	//Humm, set the SIF0 DMA channel in destination chain mode?
}

//79
void CPS2OS::sc_SifSetReg()
{
	uint32 nRegister	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 nValue		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

    m_sif.SetRegister(nRegister, nValue);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = 0;
}

//7A
void CPS2OS::sc_SifGetReg()
{
	uint32 nRegister = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(m_sif.GetRegister(nRegister));
}

//7C
void CPS2OS::sc_Deci2Call()
{
	uint32 nFunction, nParam, nID, nLength;
	uint8* sString;
	DECI2HANDLER* pHandler;

	nFunction	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	nParam		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	
	switch(nFunction)
	{
	case 0x01:
		//Deci2Open
		nID = GetNextAvailableDeci2HandlerId();

		pHandler = GetDeci2Handler(nID);
		pHandler->nValid		= 1;
		pHandler->nDevice		= *(uint32*)&m_ram[nParam + 0x00];
		pHandler->nBufferAddr	= *(uint32*)&m_ram[nParam + 0x04];

		m_ee.m_State.nGPR[SC_RETURN].nV[0] = nID;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

		break;
	case 0x03:
		//Deci2Send
		nID = *(uint32*)&m_ram[nParam + 0x00];

		pHandler = GetDeci2Handler(nID);

		if(pHandler->nValid != 0)
		{
			nParam  = *(uint32*)&m_ram[pHandler->nBufferAddr + 0x10];
			nParam &= (PS2::EERAMSIZE - 1);

			nLength = m_ram[nParam + 0x00] - 0x0C;
			sString = &m_ram[nParam + 0x0C];

            m_iopBios.GetIoman()->Write(1, nLength, sString);
		}

		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 1;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

		break;
	case 0x04:
		//Deci2Poll
		nID = *(uint32*)&m_ram[nParam + 0x00];
		
		pHandler = GetDeci2Handler(nID);
		if(pHandler->nValid != 0)
		{
			*(uint32*)&m_ram[pHandler->nBufferAddr + 0x0C] = 0;
		}

		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 1;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

		break;
	case 0x10:
		//kPuts
		nParam = *(uint32*)&m_ram[nParam];
		sString = &m_ram[nParam];
        m_iopBios.GetIoman()->Write(1, static_cast<uint32>(strlen(reinterpret_cast<char*>(sString))), sString);
		break;
	default:
		printf("PS2OS: Unknown Deci2Call function (0x%0.8X) called. PC: 0x%0.8X.\r\n", nFunction, m_ee.m_State.nPC);
		break;
	}

}

//7F
void CPS2OS::sc_GetMemorySize()
{
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = PS2::EERAMSIZE;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//////////////////////////////////////////////////
//System Call Handler
//////////////////////////////////////////////////

void CPS2OS::SysCallHandler()
{

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

    uint32 searchAddress = m_ee.m_State.nCOP0[CCOP_SCU::EPC];
    uint32 callInstruction = m_ee.m_pMemoryMap->GetWord(searchAddress);
    if(callInstruction != 0x0000000C)
    {
        throw runtime_error("Not a SYSCALL.");
    }

    uint32 nFunc;

	nFunc = m_ee.m_State.nGPR[3].nV[0];
	if(nFunc & 0x80000000)
	{
		nFunc = 0 - nFunc;
	}
	//Save for custom handler
	m_ee.m_State.nGPR[3].nV[0] = nFunc;

	if(GetCustomSyscallTable()[nFunc] == NULL)
	{
#ifdef _DEBUG
		DisassembleSysCall(static_cast<uint8>(nFunc & 0xFF));
//		if(CPS2VM::m_Logging.GetOSRecordingStatus())
//		{
//			RecordSysCall(static_cast<uint8>(nFunc & 0xFF));
//		}
#endif
		if(nFunc < 0x80)
		{
            ((this)->*(m_pSysCall[nFunc & 0xFF]))();
		}
	}
	else
	{
		m_ee.GenerateException(0x1FC00100);
	}

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif

    m_ee.m_State.nHasException = 0;
}

void CPS2OS::DisassembleSysCall(uint8 nFunc)
{
#ifdef _DEBUG
//	if(!CPS2VM::m_Logging.GetOSLoggingStatus()) return;

	string sDescription(GetSysCallDescription(nFunc));

	if(sDescription.length() != 0)
	{
//		printf("PS2OS: %s\r\n", sDescription.c_str());
        CLog::GetInstance().Print("ps2os", "%s\r\n", sDescription.c_str());
	}
#endif
}

void CPS2OS::RecordSysCall(uint8 nFunction)
{
	COsEventManager::COsEvent Event;

	string sDescription(GetSysCallDescription(nFunction));

	Event.nThreadId		= GetCurrentThreadId();
	Event.nEventType	= nFunction;
	Event.nAddress		= m_ee.m_State.nGPR[CMIPS::RA].nV0 - 8;
	Event.sDescription	= (sDescription.length() != 0) ? sDescription : ("Unknown (" + lexical_cast<string>(nFunction) + ")");

	COsEventManager::GetInstance().InsertEvent(Event);
}

string CPS2OS::GetSysCallDescription(uint8 nFunction)
{
	char sDescription[256];

	strcpy(sDescription, "");

	switch(nFunction)
	{
	case 0x02:
		sprintf(sDescription, "GsSetCrt(interlace = %i, mode = %i, field = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x11:
		sprintf(sDescription, "RemoveIntcHandler(cause = %i, id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x12:
		sprintf(sDescription, "AddDmacHandler(channel = %i, address = 0x%0.8X, next = %i, arg = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM2].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM3].nV[0]);
		break;
	case 0x13:
		sprintf(sDescription, "RemoveDmacHandler(channel = %i, handler = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x14:
		sprintf(sDescription, "EnableIntc(cause = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x15:
		sprintf(sDescription, "DisableIntc(cause = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x16:
		sprintf(sDescription, "EnableDmac(channel = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x17:
		sprintf(sDescription, "DisableDmac(channel = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x20:
		sprintf(sDescription, "CreateThread(thread = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x21:
		sprintf(sDescription, "DeleteThread(id = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x22:
		sprintf(sDescription, "StartThread(id = 0x%0.8X, a0 = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x23:
		sprintf(sDescription, "ExitThread();");
		break;
	case 0x25:
		sprintf(sDescription, "TerminateThread(id = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x29:
		sprintf(sDescription, "ChangeThreadPriority(id = 0x%0.8X, priority = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x2B:
		sprintf(sDescription, "RotateThreadReadyQueue(prio = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x2F:
		sprintf(sDescription, "GetThreadId();");
		break;
	case 0x32:
		sprintf(sDescription, "SleepThread();");
		break;
	case 0x33:
		sprintf(sDescription, "WakeupThread(id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x34:
		sprintf(sDescription, "iWakeupThread(id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x3C:
		sprintf(sDescription, "RFU060(gp = 0x%0.8X, stack = 0x%0.8X, stack_size = 0x%0.8X, args = 0x%0.8X, root_func = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM2].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM3].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM4].nV[0]);
		break;
	case 0x3D:
		sprintf(sDescription, "RFU061(heap_start = 0x%0.8X, heap_size = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x3E:
		sprintf(sDescription, "EndOfHeap();");
		break;
	case 0x40:
		sprintf(sDescription, "CreateSema(sema = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x41:
		sprintf(sDescription, "DeleteSema(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x42:
		sprintf(sDescription, "SignalSema(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x43:
		sprintf(sDescription, "iSignalSema(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x44:
		sprintf(sDescription, "WaitSema(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x45:
		sprintf(sDescription, "PollSema(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x46:
		sprintf(sDescription, "iPollSema(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x48:
		sprintf(sDescription, "iReferSemaStatus(semaid = %i, status = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x64:
	case 0x68:
#ifdef _DEBUG
//		sprintf(sDescription, "FlushCache();");
#endif
		break;
	case 0x71:
		sprintf(sDescription, "GsPutIMR(GS_IMR = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x73:
		sprintf(sDescription, "SetVSyncFlag(ptr1 = 0x%0.8X, ptr2 = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x74:
		sprintf(sDescription, "SetSyscall(num = 0x%0.2X, address = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x76:
		sprintf(sDescription, "SifDmaStat();");
		break;
	case 0x77:
		sprintf(sDescription, "SifSetDma(list = 0x%0.8X, count = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x78:
		sprintf(sDescription, "SifSetDChain();");
		break;
	case 0x79:
		sprintf(sDescription, "SifSetReg(register = 0x%0.8X, value = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7A:
		sprintf(sDescription, "SifGetReg(register = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x7C:
		sprintf(sDescription, "Deci2Call(func = 0x%0.8X, param = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7F:
		sprintf(sDescription, "GetMemorySize();");
		break;
	}

	return string(sDescription);
}

//////////////////////////////////////////////////
//System Call Handlers Table
//////////////////////////////////////////////////

//void (*CPS2OS::m_pSysCall[0x80])() = 
CPS2OS::SystemCallHandler CPS2OS::m_pSysCall[0x80] =
{
	//0x00
    &CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_GsSetCrt,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x08
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x10
	&CPS2OS::sc_AddIntcHandler,		&CPS2OS::sc_RemoveIntcHandler,		&CPS2OS::sc_AddDmacHandler,	&CPS2OS::sc_RemoveDmacHandler,		&CPS2OS::sc_EnableIntc,		&CPS2OS::sc_DisableIntc,	&CPS2OS::sc_EnableDmac,		&CPS2OS::sc_DisableDmac,
	//0x18
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x20
	&CPS2OS::sc_CreateThread,		&CPS2OS::sc_DeleteThread,			&CPS2OS::sc_StartThread,	&CPS2OS::sc_ExitThread,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_TerminateThread,&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x28
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_ChangeThreadPriority,	&CPS2OS::sc_Unhandled,		&CPS2OS::sc_RotateThreadReadyQueue,	&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_GetThreadId,
	//0x30
	&CPS2OS::sc_ReferThreadStatus,	&CPS2OS::sc_Unhandled,				&CPS2OS::sc_SleepThread,	&CPS2OS::sc_WakeupThread,			&CPS2OS::sc_WakeupThread,	&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x38
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_RFU060,			&CPS2OS::sc_RFU061,			&CPS2OS::sc_EndOfHeap,		&CPS2OS::sc_Unhandled,
	//0x40
	&CPS2OS::sc_CreateSema,			&CPS2OS::sc_DeleteSema,				&CPS2OS::sc_SignalSema,		&CPS2OS::sc_SignalSema,				&CPS2OS::sc_WaitSema,		&CPS2OS::sc_PollSema,		&CPS2OS::sc_PollSema,		&CPS2OS::sc_Unhandled,
	//0x48
	&CPS2OS::sc_ReferSemaStatus,	&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x50
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x58
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x60
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_FlushCache,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x68
	&CPS2OS::sc_FlushCache,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,
	//0x70
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_GsPutIMR,				&CPS2OS::sc_Unhandled,		&CPS2OS::sc_SetVSyncFlag,			&CPS2OS::sc_SetSyscall,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_SifDmaStat,		&CPS2OS::sc_SifSetDma,
	//0x78
	&CPS2OS::sc_SifSetDChain,		&CPS2OS::sc_SifSetReg,				&CPS2OS::sc_SifGetReg,		&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Deci2Call,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_Unhandled,		&CPS2OS::sc_GetMemorySize,
};

//////////////////////////////////////////////////
//Round Ribbon Implementation
//////////////////////////////////////////////////

CPS2OS::CRoundRibbon::CRoundRibbon(void* pMemory, uint32 nSize)
{
	NODE* pHead;

	m_pNode		= (NODE*)pMemory;
	m_nMaxNode	= nSize / sizeof(NODE);

	memset(pMemory, 0, nSize);

	pHead = GetNode(0);
	pHead->nIndexNext	= -1;
	pHead->nWeight		= -1;
	pHead->nValid		= 1;
}

CPS2OS::CRoundRibbon::~CRoundRibbon()
{

}

unsigned int CPS2OS::CRoundRibbon::Insert(uint32 nValue, uint32 nWeight)
{
	NODE* pNext;
	NODE* pPrev;
	NODE* pNode;

	//Initialize the new node
	pNode = AllocateNode();
	if(pNode == NULL) return -1;
	pNode->nWeight	= nWeight;
	pNode->nValue	= nValue;

	//Insert node in list
	pNext = GetNode(0);
	pPrev = NULL;

	while(1)
	{
		if(pNext == NULL)
		{
			//We must insert there...
			pNode->nIndexNext = pPrev->nIndexNext;
			pPrev->nIndexNext = GetNodeIndex(pNode);
			break;
		}

		if(pNext->nWeight == -1)
		{
			pPrev = pNext;
			pNext = GetNode(pNext->nIndexNext);	
			continue;
		}

		if(pNode->nWeight < pNext->nWeight)
		{
			pNext = NULL;
			continue;
		}

		pPrev = pNext;
		pNext = GetNode(pNext->nIndexNext);
	}

	return GetNodeIndex(pNode);
}

void CPS2OS::CRoundRibbon::Remove(unsigned int nIndex)
{
	NODE* pNode;
	NODE* pCurr;

	if(nIndex == 0) return;

	pCurr = GetNode(nIndex);
	if(pCurr == NULL) return;
	if(pCurr->nValid != 1) return;

	pNode = GetNode(0);

	while(1)
	{
		if(pNode == NULL) break;
        assert(pNode->nValid);

		if(pNode->nIndexNext == nIndex)
		{
			pNode->nIndexNext = pCurr->nIndexNext;
			break;
		}
		
		pNode = GetNode(pNode->nIndexNext);
	}

	FreeNode(pCurr);
}

unsigned int CPS2OS::CRoundRibbon::Begin()
{
	return GetNode(0)->nIndexNext;
}

CPS2OS::CRoundRibbon::NODE* CPS2OS::CRoundRibbon::GetNode(unsigned int nIndex)
{
	if(nIndex >= m_nMaxNode) return NULL;
	return m_pNode + nIndex;
}

unsigned int CPS2OS::CRoundRibbon::GetNodeIndex(NODE* pNode)
{
	return (unsigned int)(pNode - m_pNode);
}

CPS2OS::CRoundRibbon::NODE* CPS2OS::CRoundRibbon::AllocateNode()
{
	unsigned int i;
	NODE* pNode;

	for(i = 1; i < m_nMaxNode; i++)
	{
		pNode = GetNode(i);
		if(pNode->nValid == 1) continue;
		pNode->nValid = 1;
		return pNode;
	}

	return NULL;
}

void CPS2OS::CRoundRibbon::FreeNode(NODE* pNode)
{
	pNode->nValid = 0;
}

CPS2OS::CRoundRibbon::ITERATOR::ITERATOR(CRoundRibbon* pRibbon)
{
	m_pRibbon	= pRibbon;
	m_nIndex	= 0;
}

CPS2OS::CRoundRibbon::ITERATOR& CPS2OS::CRoundRibbon::ITERATOR::operator =(unsigned int nIndex)
{
	m_nIndex = nIndex;
	return (*this);
}

CPS2OS::CRoundRibbon::ITERATOR& CPS2OS::CRoundRibbon::ITERATOR::operator ++(int nAmount)
{
	NODE* pNode;

	if(!IsEnd())
	{
		pNode = m_pRibbon->GetNode(m_nIndex);
		m_nIndex = pNode->nIndexNext;
	}

	return (*this);
}

uint32 CPS2OS::CRoundRibbon::ITERATOR::GetValue()
{
	if(!IsEnd())
	{
		return m_pRibbon->GetNode(m_nIndex)->nValue;
	}

	return 0;
}

uint32 CPS2OS::CRoundRibbon::ITERATOR::GetWeight()
{
	if(!IsEnd())
	{
		return m_pRibbon->GetNode(m_nIndex)->nWeight;
	}

	return -1;
}

unsigned int CPS2OS::CRoundRibbon::ITERATOR::GetIndex()
{
	return m_nIndex;
}

bool CPS2OS::CRoundRibbon::ITERATOR::IsEnd()
{
	if(m_pRibbon == NULL) return true;
	return m_pRibbon->GetNode(m_nIndex) == NULL;
}
