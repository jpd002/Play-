#include <stddef.h>
#include <stdlib.h>
#include <exception>
#include <boost/filesystem/path.hpp>
#include "PS2OS.h"
#include "PS2VM.h"
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

#define CONFIGPATH ".\\config\\"
#define PATCHESPATH "patches.xml"

#define THREAD_INIT_QUOTA			(15)

using namespace Framework;
using namespace std;
using namespace boost;

signal<void ()>						CPS2OS::m_OnExecutableChange;
signal<void ()>						CPS2OS::m_OnExecutableUnloading;

bool								CPS2OS::m_nInitialized		= false;
CELF*								CPS2OS::m_pELF				= NULL;

string								CPS2OS::m_sExecutableName;
CMIPS*								CPS2OS::m_pCtx				= NULL;

CPS2OS::CRoundRibbon*				CPS2OS::m_pThreadSchedule	= NULL;

CPS2OS::CPS2OS()
{
	if(m_nInitialized)
	{
		assert(0);
	}
	Initialize();
}

CPS2OS::~CPS2OS()
{
	if(!m_nInitialized)
	{
		assert(0);
	}
	Release();
}

void CPS2OS::Initialize()
{
	m_pELF = NULL;
	m_pCtx = &CPS2VM::m_EE;

	m_pCtx->m_State.nGPR[CMIPS::K0].nV[0] = 0x80030000;
	m_pCtx->m_State.nGPR[CMIPS::K0].nV[1] = 0xFFFFFFFF;

	m_pThreadSchedule = new CRoundRibbon(CPS2VM::m_pRAM + 0x30000, 0x2000);

	m_nInitialized = true;
}

void CPS2OS::Release()
{
	UnloadExecutable();
	
	DELETEPTR(m_pThreadSchedule);

	m_nInitialized = false;
}

bool CPS2OS::IsInitialized()
{
	return m_nInitialized;
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

		printf("ID: %0.2i, Priority: %0.2i, EPC: 0x%0.8X, Status: %s, WaitSema: %i.\r\n", \
			itThread.GetValue(), \
			pThread->nPriority, \
			pThread->nEPC, \
			sStatus, \
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
	LoadELF(new CStdStream(fopen(ExecPath.string().c_str(), "rb")), ExecPath.leaf().c_str());
}

void CPS2OS::BootFromCDROM()
{
	CStream* pFile;
	CStrA sLine;
	const char* sExecPath;
	const char* sExecName;

	pFile = CSIF::GetFileIO()->GetFile(IOP::CFileIO::O_RDONLY, "cdrom0:SYSTEM.CNF");
	if(pFile == NULL)
	{
		throw exception("No 'SYSTEM.CNF' file found on the cdrom0 device.");
	}

	sExecPath = NULL;

	Utils::GetLine(pFile, &sLine);
	while(!pFile->IsEOF())
	{
		if(!strncmp(sLine, "BOOT2", 5))
		{
			sExecPath = strstr(sLine, "=");
			if(sExecPath != NULL)
			{
				sExecPath++;
				if(sExecPath[0] == ' ') sExecPath++;
				break;
			}
		}
		Utils::GetLine(pFile, &sLine);
	}

	delete pFile;

	if(sExecPath == NULL)
	{
		throw exception("Error parsing 'SYSTEM.CNF' for a BOOT2 value.");
	}

	pFile = CSIF::GetFileIO()->GetFile(IOP::CFileIO::O_RDONLY, sExecPath);

	sExecName = strchr(sExecPath, ':') + 1;
	if(sExecName[0] == '/' || sExecName[0] == '\\') sExecName++;

	LoadELF(pFile, sExecName);
}

CELF* CPS2OS::GetELF()
{
	return m_pELF;
}

const char* CPS2OS::GetExecutableName()
{
	return m_sExecutableName.c_str();
}

void CPS2OS::LoadELF(CStream* pStream, const char* sExecName)
{
	CELF* pELF;

	try
	{
		pELF = new CELF(pStream);
		delete pStream;
	}
	catch(const exception& Exception)
	{
		delete pStream;
		throw Exception;
	}

	//Check for MIPS CPU
	if(pELF->m_Header.nCPU != 8)
	{
		DELETEPTR(pELF);
		throw exception("Invalid target CPU. Must be MIPS.");
	}

	if(pELF->m_Header.nType != 2)
	{
		DELETEPTR(pELF);
		throw exception("Not an executable ELF file.");
	}
	
	CPS2VM::Pause();

	UnloadExecutable();

	m_pELF = pELF;

	m_sExecutableName = sExecName;

	LoadExecutable();

	//Just a test
//	pStream = new CStdStream(fopen("./vfs/host/sjpcm.irx", "rb"));
//	pStream = new CStdStream(fopen("./vfs/host/padman.irx", "rb"));
//	pELF = new CELF(pStream);
//	memcpy(CPS2VM::m_pRAM + 0x01000000, pELF->m_pData, pELF->m_nLenght);
//	delete pELF;
/*
	int i;
	uint32 nVal;
	for(i = 0; i < 0x02000000 / 4; i++)
	{
		nVal = ((uint32*)CPS2VM::m_pRAM)[i];
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
		nVal = ((uint32*)CPS2VM::m_pRAM)[i];
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
//	*((uint32*)&CPS2VM::m_pRAM[0x0010B5E4]) = 0;
//	*((uint32*)&CPS2VM::m_pRAM[0x002F6C58]) = 0;
//	*((uint32*)&CPS2VM::m_pRAM[0x00109208]) = 0x28840100;
//	*((uint32*)&CPS2VM::m_pRAM[0x0010922C]) = 0;
//	*((uint32*)&CPS2VM::m_pRAM[0x001067C4]) = 0;
//	*((uint32*)&CPS2VM::m_pRAM[0x001AC028]) = 0;

//	*(uint32*)&CPS2VM::m_pRAM[0x0029B758] = 0;
//	*(uint32*)&CPS2VM::m_pRAM[0x0029B768] = 0;
//	*(uint32*)&CPS2VM::m_pRAM[0x0029B774] = 0;

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

	CPS2VM::m_EE.InvalidateCache();

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
			memcpy(CPS2VM::m_pRAM + p->nVAddress, (uint8*)m_pELF->m_pData + p->nOffset, p->nFileSize);
		}
	}

	//Load the comments maybe?
	LoadExecutableConfig();

	//InsertFunctionSymbols();

	CPS2VM::m_EE.m_State.nPC = m_pELF->m_Header.nEntryPoint;
	
	//Install hooks
	CPS2VM::m_EE.m_pSysCallHandler = SysCallHandler;
	*(uint32*)&CPS2VM::m_pBIOS[0x00000004] = 0x0000001D;
	AssembleCustomSyscallHandler();
	AssembleInterruptHandler();
	AssembleDmacHandler();
	AssembleIntcHandler();
	AssembleThreadEpilog();
	AssembleWaitThreadProc();
	CreateWaitThread();

#ifdef DEBUGGER_INCLUDED
	CPS2VM::m_EE.m_pAnalysis->Clear();
	CPS2VM::m_EE.m_pAnalysis->Analyse(nMinAddr, (nMinAddr + m_pELF->m_nLenght) & ~0x3);
#endif

	CPS2VM::m_OnMachineStateChange();
}

void CPS2OS::UnloadExecutable()
{
	m_OnExecutableUnloading();
	COsEventManager::GetInstance().Flush();

	if(m_pELF == NULL) return;

	DELETEPTR(m_pELF);

	SaveExecutableConfig();

	CPS2VM::m_EE.m_Comments.RemoveTags();
	CPS2VM::m_EE.m_Functions.RemoveTags();
}

void CPS2OS::LoadExecutableConfig()
{

#ifdef DEBUGGER_INCLUDED

	string sPath;

	//Functions
	sPath = CONFIGPATH + m_sExecutableName + ".functions";
	CPS2VM::m_EE.m_Functions.Unserialize(sPath.c_str());

	//Comments
	sPath = CONFIGPATH + m_sExecutableName + ".comments";
	CPS2VM::m_EE.m_Comments.Unserialize(sPath.c_str());

	//VU1 Comments
	sPath = CONFIGPATH + m_sExecutableName + ".vu1comments";
	CPS2VM::m_VU1.m_Comments.Unserialize(sPath.c_str());

#endif

}

void CPS2OS::SaveExecutableConfig()
{

#ifdef DEBUGGER_INCLUDED

	string sPath;

	//Functions
	sPath = CONFIGPATH + m_sExecutableName + ".functions";
	CPS2VM::m_EE.m_Functions.Serialize(sPath.c_str());

	//Comments
	sPath = CONFIGPATH + m_sExecutableName + ".comments";
	CPS2VM::m_EE.m_Comments.Serialize(sPath.c_str());

	//VU1 Comments
	sPath = CONFIGPATH + m_sExecutableName + ".vu1comments";
	CPS2VM::m_VU1.m_Comments.Serialize(sPath.c_str());

#endif

}

void CPS2OS::ApplyPatches()
{
	Xml::CNode* pDocument;
	Xml::CNode* pPatches;

	try
	{
		pDocument = Xml::CParser::ParseDocument(&CStdStream(fopen(PATCHESPATH, "rb")));
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

				*(uint32*)&CPS2VM::m_pRAM[nAddress] = nValue;

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
	CMIPSAssembler Asm((uint32*)&CPS2VM::m_pBIOS[0x100]);

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
	CMIPSAssembler Asm((uint32*)&CPS2VM::m_pBIOS[0x200]);
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
	CMIPSAssembler Asm((uint32*)&CPS2VM::m_pBIOS[0x1000]);

	//Prologue
	//S0 -> Channel Counter
	//S1 -> DMA Interrupt Status
	//S2 -> Handler Counter

	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE0);
	Asm.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	Asm.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	Asm.SD(CMIPS::S1, 0x0010, CMIPS::SP);
	Asm.SD(CMIPS::S2, 0x0018, CMIPS::SP);

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
	CMIPSAssembler Asm((uint32*)&CPS2VM::m_pBIOS[0x2000]);

	//Prologue
	//S0 -> Handler Counter

	Asm.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE8);
	Asm.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	Asm.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	Asm.SD(CMIPS::S1, 0x0010, CMIPS::SP);

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
	CMIPSAssembler Asm((uint32*)&CPS2VM::m_pBIOS[0x3000]);
	
	Asm.ADDIU(CMIPS::V1, CMIPS::R0, 0x23);
	Asm.SYSCALL();
}

void CPS2OS::AssembleWaitThreadProc()
{
	CMIPSAssembler Asm((uint32*)&CPS2VM::m_pBIOS[BIOS_ADDRESS_WAITTHREADPROC - BIOS_ADDRESS_BASE]);

//	Asm.ADDIU(CMIPS::V1, CMIPS::R0, 0x03);
//	Asm.SYSCALL();

	Asm.BEQ(CMIPS::R0, CMIPS::R0, 0xFFFF);
	Asm.NOP();
}

uint32* CPS2OS::GetCustomSyscallTable()
{
	return (uint32*)&CPS2VM::m_pRAM[0x00010000];
}

uint32 CPS2OS::GetCurrentThreadId()
{
	return *(uint32*)&CPS2VM::m_pRAM[0x00000000];
}

void CPS2OS::SetCurrentThreadId(uint32 nThread)
{
	*(uint32*)&CPS2VM::m_pRAM[0x00000000] = nThread;
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
	return &((THREAD*)&CPS2VM::m_pRAM[0x00011000])[nID];
}

void CPS2OS::ThreadShakeAndBake()
{
	THREAD* pThread;
	unsigned int nId;

	//Don't play with fire (don't switch if we're in exception mode)
	if(CPS2VM::m_EE.m_State.nCOP0[CCOP_SCU::STATUS] & 0x02)
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
		if(pThread->nQuota == 0) continue;
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
		m_pThreadSchedule->Insert(nId, pThread->nPriority);
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
	pContext = (THREADCONTEXT*)&CPS2VM::m_pRAM[pThread->nContextPtr];

	//Save the context
	for(uint32 i = 0; i < 0x20; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		pContext->nGPR[i] = m_pCtx->m_State.nGPR[i];
	}

	pThread->nEPC = m_pCtx->m_State.nPC;

	SetCurrentThreadId(nID);
	pThread = GetThread(GetCurrentThreadId());
	pContext = (THREADCONTEXT*)&CPS2VM::m_pRAM[pThread->nContextPtr];

	//Load the context

	m_pCtx->m_State.nPC = pThread->nEPC;

	for(uint32 i = 0; i < 0x20; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		m_pCtx->m_State.nGPR[i] = pContext->nGPR[i];
	}

	if(CPS2VM::m_Logging.GetOSLoggingStatus())
	{
		printf("PS2OS: New thread elected (id = %i).\r\n", nID);
	}
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
	return &((SEMAPHORE*)&CPS2VM::m_pRAM[0x0000E000])[nID];
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
	return &((DMACHANDLER*)&CPS2VM::m_pRAM[0x0000C000])[nID];
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
	return &((INTCHANDLER*)&CPS2VM::m_pRAM[0x0000A000])[nID];
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
	return &((DECI2HANDLER*)&CPS2VM::m_pRAM[0x00008000])[nID];
}

void CPS2OS::ExceptionHandler()
{
	ThreadShakeAndBake();
	m_pCtx->GenerateInterrupt(0x1FC00200);
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
	printf("PS2OS: Unknown system call (%X) called from 0x%0.8X.\r\n", m_pCtx->m_State.nGPR[3].nV[0], m_pCtx->m_State.nPC);
}

//02
void CPS2OS::sc_GsSetCrt()
{
	unsigned int nMode;
	bool nIsInterlaced;
	bool nIsFrameMode;

	nIsInterlaced	= (m_pCtx->m_State.nGPR[SC_PARAM0].nV[0] != 0);
	nMode			= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];
	nIsFrameMode	= (m_pCtx->m_State.nGPR[SC_PARAM2].nV[0] != 0);

	CPS2VM::m_pGS->SetCrt(nIsInterlaced, nMode, nIsFrameMode);
}

//10
void CPS2OS::sc_AddIntcHandler()
{
	uint32 nAddress, nCause, nNext, nArg, nID;
	INTCHANDLER* pHandler;

	nCause		= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nAddress	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];
	nNext		= m_pCtx->m_State.nGPR[SC_PARAM2].nV[0];
	nArg		= m_pCtx->m_State.nGPR[SC_PARAM3].nV[0];

	/*
	if(nNext != 0)
	{
		assert(0);
	}
	*/

	nID = GetNextAvailableIntcHandlerId();
	if(nID == 0xFFFFFFFF)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pHandler = GetIntcHandler(nID);
	pHandler->nValid	= 1;
	pHandler->nAddress	= nAddress;
	pHandler->nCause	= nCause;
	pHandler->nArg		= nArg;
	pHandler->nGP		= m_pCtx->m_State.nGPR[CMIPS::GP].nV[0];

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//11
void CPS2OS::sc_RemoveIntcHandler()
{
	uint32 nCause, nID;
	INTCHANDLER* pHandler;

	nCause		= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nID			= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	pHandler = GetIntcHandler(nID);
	if(pHandler->nValid != 1)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pHandler->nValid = 0;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//12
void CPS2OS::sc_AddDmacHandler()
{
	uint32 nAddress, nChannel, nNext, nArg, nID;
	DMACHANDLER* pHandler;

	nChannel	= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nAddress	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];
	nNext		= m_pCtx->m_State.nGPR[SC_PARAM2].nV[0];
	nArg		= m_pCtx->m_State.nGPR[SC_PARAM3].nV[0];

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
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pHandler = GetDmacHandler(nID);
	pHandler->nValid	= 1;
	pHandler->nAddress	= nAddress;
	pHandler->nChannel	= nChannel;
	pHandler->nArg		= nArg;
	pHandler->nGP		= m_pCtx->m_State.nGPR[CMIPS::GP].nV[0];

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//13
void CPS2OS::sc_RemoveDmacHandler()
{
	uint32 nChannel, nID;
	DMACHANDLER* pHandler;

	nChannel	= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nID			= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	pHandler = GetDmacHandler(nID);
	pHandler->nValid = 0x00;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//14
void CPS2OS::sc_EnableIntc()
{
	uint32 nCause, nMask;

	nCause = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	nMask = 1 << nCause;
	if(!(CINTC::GetRegister(CINTC::INTC_MASK) & nMask))
	{
		CINTC::SetRegister(CINTC::INTC_MASK, nMask);
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 1;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//15
void CPS2OS::sc_DisableIntc()
{
	uint32 nCause, nMask;

	nCause = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	nMask = 1 << nCause;
	if(CINTC::GetRegister(CINTC::INTC_MASK) & nMask)
	{
		CINTC::SetRegister(CINTC::INTC_MASK, nMask);
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 1;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//16
void CPS2OS::sc_EnableDmac()
{
	uint32 nChannel, nRegister;

	nChannel = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	nRegister = 0x10000 << nChannel;

	if(!(CDMAC::GetRegister(CDMAC::D_STAT) & nRegister))
	{
		CDMAC::SetRegister(CDMAC::D_STAT, nRegister);
	}

	//Enable INT1
	if(!(CINTC::GetRegister(CINTC::INTC_MASK) & 0x02))
	{
		CINTC::SetRegister(CINTC::INTC_MASK, 0x02);
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 1;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//17
void CPS2OS::sc_DisableDmac()
{
	uint32 nChannel, nRegister;

	nChannel = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	nRegister = 0x10000 << nChannel;

	if(CDMAC::GetRegister(CDMAC::D_STAT) & nRegister)
	{
		CDMAC::SetRegister(CDMAC::D_STAT, nRegister);
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 1;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//20
void CPS2OS::sc_CreateThread()
{
	THREADPARAM* pThreadParam;
	THREAD* pThread;
	THREADCONTEXT* pContext;
	uint32 nID, nStackAddr, nHeapBase;

	pThreadParam = (THREADPARAM*)&CPS2VM::m_pRAM[m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]];

	nID = GetNextAvailableThreadId();
	if(nID == 0xFFFFFFFF)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
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

	pContext = (THREADCONTEXT*)&CPS2VM::m_pRAM[pThread->nContextPtr];
	memset(pContext, 0, sizeof(THREADCONTEXT));

	pContext->nGPR[CMIPS::SP].nV0 = nStackAddr;
	pContext->nGPR[CMIPS::FP].nV0 = nStackAddr;
	pContext->nGPR[CMIPS::GP].nV0 = pThreadParam->nGP;
	pContext->nGPR[CMIPS::RA].nV0 = 0x1FC03000;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//21
void CPS2OS::sc_DeleteThread()
{
	uint32 nID;
	THREAD* pThread;

	nID = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	m_pThreadSchedule->Remove(pThread->nScheduleID);

	pThread->nValid = 0;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//22
void CPS2OS::sc_StartThread()
{
	uint32 nID, nArg;
	THREAD* pThread;
	THREADCONTEXT* pContext;

	nID		= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nArg	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pThread->nStatus = THREAD_RUNNING;

	pContext = (THREADCONTEXT*)&CPS2VM::m_pRAM[pThread->nContextPtr];
	pContext->nGPR[CMIPS::A0].nV0 = nArg;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
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

	nID = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pThread->nStatus = THREAD_ZOMBIE;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//29
void CPS2OS::sc_ChangeThreadPriority()
{
	uint32 nID, nPrio, nPrevPrio;
	THREAD* pThread;

	nID		= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nPrio	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	nPrevPrio = pThread->nPriority;
	pThread->nPriority = nPrio;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nPrevPrio;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

	//Reschedule?
	m_pThreadSchedule->Remove(pThread->nScheduleID);
	pThread->nScheduleID = m_pThreadSchedule->Insert(nID, pThread->nPriority);

	ThreadShakeAndBake();
}

//2B
void CPS2OS::sc_RotateThreadReadyQueue()
{
	uint32 nPrio, nID;
	CRoundRibbon::ITERATOR itThread(m_pThreadSchedule);

	nPrio = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

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

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nPrio;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

	if(!itThread.IsEnd())
	{
		//Change has been made
		ThreadShakeAndBake();
	}
}

//2F
void CPS2OS::sc_GetThreadId()
{
	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = GetCurrentThreadId();
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//30
void CPS2OS::sc_ReferThreadStatus()
{
	//THS_RUN = 0x01, THS_READY = 0x02, THS_WAIT = 0x04, THS_SUSPEND = 0x08, THS_DORMANT = 0x10
	uint32 nID, nStatusPtr, nRet;
	THREAD* pThread;

	nID			= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nStatusPtr	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	pThread = GetThread(nID);
	if(!pThread->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
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

		pThreadParam = (THREADPARAM*)(&CPS2VM::m_pRAM[nStatusPtr]);
		pThreadParam->nStatus		= nRet;
		pThreadParam->nPriority		= pThread->nPriority;
		pThreadParam->nStackBase	= pThread->nStackBase;
		pThreadParam->nStackSize	= pThread->nStackSize;
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nRet;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
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
	THREAD* pThread;
	uint32 nID;
	bool nInt;

	nID		= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nInt	= m_pCtx->m_State.nGPR[3].nV[0] == 0x34;

	pThread = GetThread(nID);

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

	nStackBase = m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];
	nStackSize = m_pCtx->m_State.nGPR[SC_PARAM2].nV[0];

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

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nStackAddr;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//3D
void CPS2OS::sc_RFU061()
{
	uint32 nHeapBase, nHeapSize;
	THREAD* pThread;

	pThread = GetThread(GetCurrentThreadId());

	nHeapBase = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nHeapSize = m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	if(nHeapSize == 0xFFFFFFFF)
	{
		pThread->nHeapBase = pThread->nStackBase;
	}
	else
	{
		pThread->nHeapBase = nHeapBase + nHeapSize;
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = pThread->nHeapBase;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//3E
void CPS2OS::sc_EndOfHeap()
{
	THREAD* pThread;

	pThread = GetThread(GetCurrentThreadId());

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = pThread->nHeapBase;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//40
void CPS2OS::sc_CreateSema()
{
	SEMAPHOREPARAM* pSemaParam;
	SEMAPHORE* pSema;
	uint32 nID;

	pSemaParam = (SEMAPHOREPARAM*)(CPS2VM::m_pRAM + m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);

	nID = GetNextAvailableSemaphoreId();
	if(nID == 0xFFFFFFFF)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pSema = GetSemaphore(nID);
	pSema->nValid		= 1;
	pSema->nCount		= pSemaParam->nInitCount;
	pSema->nMaxCount	= pSemaParam->nMaxCount;
	pSema->nWaitCount	= 0;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//41
void CPS2OS::sc_DeleteSema()
{
	uint32 nID;
	SEMAPHORE* pSema;

	nID = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	//Check if any threads are waiting for this?
	if(pSema->nWaitCount != 0)
	{
		assert(0);
	}

	pSema->nValid = 0;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//42
//43
void CPS2OS::sc_SignalSema()
{
	bool nInt;
	uint32 nID, i;
	SEMAPHORE* pSema;
	THREAD* pThread;

	nInt	= m_pCtx->m_State.nGPR[3].nV[0] == 0x43;
	nID		= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
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

		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

		if(!nInt)
		{
			ThreadShakeAndBake();
		}
	}
	else
	{
		pSema->nCount++;
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//44
void CPS2OS::sc_WaitSema()
{
	SEMAPHORE* pSema;
	THREAD* pThread;
	uint32 nID;

	nID = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
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

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

	//REMOVE
	//Force reschedule
	//nID = GetNextReadyThread();
	//if(nID != GetCurrentThreadId())
	//{
	//	ElectThread(nID);
	//}
}

//45
void CPS2OS::sc_PollSema()
{
	uint32 nID;
	SEMAPHORE* pSema;

	nID = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	if(pSema->nCount == 0)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pSema->nCount--;
	
	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//48
void CPS2OS::sc_ReferSemaStatus()
{
	uint32 nID;
	SEMAPHORE* pSema;
	SEMAPHOREPARAM *pSemaParam;

	nID = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	pSemaParam = (SEMAPHOREPARAM*)(CPS2VM::m_pRAM + (m_pCtx->m_State.nGPR[SC_PARAM1].nV[0] & 0x1FFFFFFF));

	pSema = GetSemaphore(nID);
	if(!pSema->nValid)
	{
		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	pSemaParam->nCount			= pSema->nCount;
	pSemaParam->nMaxCount		= pSema->nMaxCount;
	pSemaParam->nWaitThreads	= pSema->nWaitCount;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//64
void CPS2OS::sc_FlushCache()
{

}

//71
void CPS2OS::sc_GsPutIMR()
{
	uint32 nIMR;

	nIMR = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	CPS2VM::m_pGS->WritePrivRegister(CGSHandler::GS_IMR, nIMR);
}

//73
void CPS2OS::sc_SetVSyncFlag()
{
	uint32 nPtr1, nPtr2;
	CGSHandler* pGsHandler;

	nPtr1	= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nPtr2	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	*(uint32*)&CPS2VM::m_pRAM[nPtr1] = 0x01;

	pGsHandler = CPS2VM::GetGSHandler();
	if(pGsHandler != NULL)
	{
		//*(uint32*)&CPS2VM::m_pRAM[nPtr2] = 0x2000;
		*(uint32*)&CPS2VM::m_pRAM[nPtr2] = pGsHandler->ReadPrivRegister(CGSHandler::GS_CSR) & 0x2000;
	}
	else
	{
		//Humm...
		*(uint32*)&CPS2VM::m_pRAM[nPtr2] = 0;
	}

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

	//REMOVE
	//Force reschedule
	//ElectThread(GetNextReadyThread());
}

//74
void CPS2OS::sc_SetSyscall()
{
	uint32 nAddress;
	uint8 nNumber;

	nNumber		= (uint8)(m_pCtx->m_State.nGPR[SC_PARAM0].nV[0] & 0xFF);
	nAddress	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	GetCustomSyscallTable()[nNumber]	= nAddress;

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//76
void CPS2OS::sc_SifDmaStat()
{
	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
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

	DMAREG* pXfer;
	uint32 nCount, i, nSize;

	pXfer	= (DMAREG*)(CPS2VM::m_pRAM + m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
	nCount	= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	//Returns count
	//DMA might call an interrupt handler
	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nCount;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

	//REMOVE
	//Force reschedule
	//ThreadShakeAndBake();

	for(i = 0; i < nCount; i++)
	{
		nSize = (pXfer[i].nSize + 0x0F) / 0x10;

		CDMAC::SetRegister(CDMAC::D6_MADR,	pXfer[i].nSrcAddr);
		CDMAC::SetRegister(CDMAC::D6_TADR,	pXfer[i].nDstAddr);
		CDMAC::SetRegister(CDMAC::D6_QWC,	nSize);
		CDMAC::SetRegister(CDMAC::D6_CHCR,	0x00000100);
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
	uint32 nRegister, nValue;

	nRegister	= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nValue		= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];

	CSIF::SetRegister(nRegister, nValue);

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//7A
void CPS2OS::sc_SifGetReg()
{
	uint32 nRegister;

	nRegister = m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];

	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = CSIF::GetRegister(nRegister);
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//7C
void CPS2OS::sc_Deci2Call()
{
	uint32 nFunction, nParam, nID, nLength;
	uint8* sString;
	DECI2HANDLER* pHandler;

	nFunction	= m_pCtx->m_State.nGPR[SC_PARAM0].nV[0];
	nParam		= m_pCtx->m_State.nGPR[SC_PARAM1].nV[0];
	
	switch(nFunction)
	{
	case 0x01:
		//Deci2Open
		nID = GetNextAvailableDeci2HandlerId();

		pHandler = GetDeci2Handler(nID);
		pHandler->nValid		= 1;
		pHandler->nDevice		= *(uint32*)&CPS2VM::m_pRAM[nParam + 0x00];
		pHandler->nBufferAddr	= *(uint32*)&CPS2VM::m_pRAM[nParam + 0x04];

		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = nID;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

		break;
	case 0x03:
		//Deci2Send
		nID = *(uint32*)&CPS2VM::m_pRAM[nParam + 0x00];

		pHandler = GetDeci2Handler(nID);

		if(pHandler->nValid != 0)
		{
			nParam  = *(uint32*)&CPS2VM::m_pRAM[pHandler->nBufferAddr + 0x10];
			nParam &= (CPS2VM::RAMSIZE - 1);

			nLength = CPS2VM::m_pRAM[nParam + 0x00] - 0x0C;
			sString = &CPS2VM::m_pRAM[nParam + 0x0C];

			CSIF::GetFileIO()->Write(1, nLength, sString);
		}

		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 1;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

		break;
	case 0x04:
		//Deci2Poll
		nID = *(uint32*)&CPS2VM::m_pRAM[nParam + 0x00];
		
		pHandler = GetDeci2Handler(nID);
		if(pHandler->nValid != 0)
		{
			*(uint32*)&CPS2VM::m_pRAM[pHandler->nBufferAddr + 0x0C] = 0;
		}

		m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = 1;
		m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;

		break;
	case 0x10:
		//kPuts
		nParam = *(uint32*)&CPS2VM::m_pRAM[nParam];
		sString = &CPS2VM::m_pRAM[nParam];
		CSIF::GetFileIO()->Write(1, (uint32)strlen((char*)sString), sString);
		break;
	default:
		printf("PS2OS: Unknown Deci2Call function (0x%0.8X) called. PC: 0x%0.8X.\r\n", nFunction, m_pCtx->m_State.nPC);
		break;
	}

}

//7F
void CPS2OS::sc_GetMemorySize()
{
	m_pCtx->m_State.nGPR[SC_RETURN].nV[0] = CPS2VM::RAMSIZE;
	m_pCtx->m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//////////////////////////////////////////////////
//System Call Handler
//////////////////////////////////////////////////

void CPS2OS::SysCallHandler()
{

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	uint32 nFunc;

	nFunc = m_pCtx->m_State.nGPR[3].nV[0];
	if(nFunc & 0x80000000)
	{
		nFunc = 0 - nFunc;
	}
	//Save for custom handler
	m_pCtx->m_State.nGPR[3].nV[0] = nFunc;

	if(GetCustomSyscallTable()[nFunc] == NULL)
	{
#ifdef _DEBUG
		DisassembleSysCall(static_cast<uint8>(nFunc & 0xFF));
		RecordSysCall(static_cast<uint8>(nFunc & 0xFF));
#endif
		if(nFunc < 0x80)
		{
			m_pSysCall[nFunc & 0xFF]();
		}
	}
	else
	{
		m_pCtx->GenerateException(0x1FC00100);
	}

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif

}

void CPS2OS::DisassembleSysCall(uint8 nFunc)
{
	if(!CPS2VM::m_Logging.GetOSLoggingStatus()) return;

	switch(nFunc)
	{
	case 0x02:
		printf("PS2OS: GsSetCrt(interlace = %i, mode = %i, field = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x11:
		printf("PS2OS: RemoveIntcHandler(cause = %i, id = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x12:
		printf("PS2OS: AddDmacHandler(channel = %i, address = 0x%0.8X, next = %i, arg = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM2].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM3].nV[0]);
		break;
	case 0x13:
		printf("PS2OS: RemoveDmacHandler(channel = %i, handler = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x14:
		printf("PS2OS: EnableIntc(cause = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x15:
		printf("PS2OS: DisableIntc(cause = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x16:
		printf("PS2OS: EnableDmac(channel = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x17:
		printf("PS2OS: DisableDmac(channel = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x20:
		printf("PS2OS: CreateThread(thread = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x21:
		printf("PS2OS: DeleteThread(id = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x22:
		printf("PS2OS: StartThread(id = 0x%0.8X, a0 = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x23:
		printf("PS2OS: ExitThread();\r\n");
		break;
	case 0x25:
		printf("PS2OS: TerminateThread(id = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x29:
		printf("PS2OS: ChangeThreadPriority(id = 0x%0.8X, priority = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x2B:
		printf("PS2OS: RotateThreadReadyQueue(prio = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x2F:
		printf("PS2OS: GetThreadId();\r\n");
		break;
	case 0x32:
		printf("PS2OS: SleepThread();\r\n");
		break;
	case 0x33:
		printf("PS2OS: WakeupThread(id = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x34:
		printf("PS2OS: iWakeupThread(id = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x3C:
		printf("PS2OS: RFU060(gp = 0x%0.8X, stack = 0x%0.8X, stack_size = 0x%0.8X, args = 0x%0.8X, root_func = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM2].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM3].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM4].nV[0]);
		break;
	case 0x3D:
		printf("PS2OS: RFU061(heap_start = 0x%0.8X, heap_size = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x3E:
		printf("PS2OS: EndOfHeap();\r\n");
		break;
	case 0x40:
		printf("PS2OS: CreateSema(sema = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x41:
		printf("PS2OS: DeleteSema(semaid = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x42:
		printf("PS2OS: SignalSema(semaid = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x43:
		printf("PS2OS: iSignalSema(semaid = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x44:
		printf("PS2OS: WaitSema(semaid = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x45:
		printf("PS2OS: PollSema(semaid = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x46:
		printf("PS2OS: iPollSema(semaid = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x48:
		printf("PS2OS: iReferSemaStatus(semaid = %i, status = 0x%0.8X);\r\n",
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0],
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x64:
	case 0x68:
#ifdef _DEBUG
//		printf("FlushCache();\r\n");
#endif
		break;
	case 0x71:
		printf("PS2OS: GsPutIMR(GS_IMR = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x73:
		printf("PS2OS: SetVSyncFlag(ptr1 = 0x%0.8X, ptr2 = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x74:
		printf("PS2OS: SetSyscall(num = 0x%0.2X, address = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x76:
		printf("PS2OS: SifDmaStat();\r\n");
		break;
	case 0x77:
		printf("PS2OS: SifSetDma(list = 0x%0.8X, count = %i);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x78:
		printf("PS2OS: SifSetDChain();\r\n");
		break;
	case 0x79:
		printf("PS2OS: SifSetReg(register = 0x%0.8X, value = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7A:
		printf("PS2OS: SifGetReg(register = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x7C:
		printf("PS2OS: Deci2Call(func = 0x%0.8X, param = 0x%0.8X);\r\n", \
			m_pCtx->m_State.nGPR[SC_PARAM0].nV[0], \
			m_pCtx->m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7F:
		printf("PS2OS: GetMemorySize();\r\n");
		break;
	}
}

void CPS2OS::RecordSysCall(uint8 nFunction)
{
	COsEventManager::OSEVENT Event;
	
	Event.nThreadId		= GetCurrentThreadId();
	Event.nEventType	= nFunction;

	COsEventManager::GetInstance().InsertEvent(Event);
}

//////////////////////////////////////////////////
//System Call Handlers Table
//////////////////////////////////////////////////

void (*CPS2OS::m_pSysCall[0x80])() = 
{
	//0x00
	sc_Unhandled,			sc_Unhandled,				sc_GsSetCrt,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x08
	sc_Unhandled,			sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x10
	sc_AddIntcHandler,		sc_RemoveIntcHandler,		sc_AddDmacHandler,	sc_RemoveDmacHandler,		sc_EnableIntc,		sc_DisableIntc,		sc_EnableDmac,		sc_DisableDmac,
	//0x18
	sc_Unhandled,			sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x20
	sc_CreateThread,		sc_DeleteThread,			sc_StartThread,		sc_ExitThread,				sc_Unhandled,		sc_TerminateThread,	sc_Unhandled,		sc_Unhandled,
	//0x28
	sc_Unhandled,			sc_ChangeThreadPriority,	sc_Unhandled,		sc_RotateThreadReadyQueue,	sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_GetThreadId,
	//0x30
	sc_ReferThreadStatus,	sc_Unhandled,				sc_SleepThread,		sc_WakeupThread,			sc_WakeupThread,	sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x38
	sc_Unhandled,			sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_RFU060,			sc_RFU061,			sc_EndOfHeap,		sc_Unhandled,
	//0x40
	sc_CreateSema,			sc_DeleteSema,				sc_SignalSema,		sc_SignalSema,				sc_WaitSema,		sc_PollSema,		sc_PollSema,		sc_Unhandled,
	//0x48
	sc_ReferSemaStatus,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x50
	sc_Unhandled,			sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x58
	sc_Unhandled,			sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x60
	sc_Unhandled,			sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_FlushCache,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x68
	sc_FlushCache,			sc_Unhandled,				sc_Unhandled,		sc_Unhandled,				sc_Unhandled,		sc_Unhandled,		sc_Unhandled,		sc_Unhandled,
	//0x70
	sc_Unhandled,			sc_GsPutIMR,				sc_Unhandled,		sc_SetVSyncFlag,			sc_SetSyscall,		sc_Unhandled,		sc_SifDmaStat,		sc_SifSetDma,
	//0x78
	sc_SifSetDChain,		sc_SifSetReg,				sc_SifGetReg,		sc_Unhandled,				sc_Deci2Call,		sc_Unhandled,		sc_Unhandled,		sc_GetMemorySize,
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
