#include <stdio.h>
#include <exception>
#include "PS2VM.h"
#include "PS2OS.h"
#include "Ps2Const.h"
#include "VIF.h"
#include "Timer.h"
#include "MA_EE.h"
#include "MA_VU.h"
#include "COP_SCU.h"
#include "COP_FPU.h"
#include "COP_VU.h"
#include "PtrMacro.h"
#include "StdStream.h"
#include "GZipStream.h"
#ifdef WIN32
#include "VolumeStream.h"
#else
#include "Posix_VolumeStream.h"
#endif
#include "stricmp.h"
#include "IszImageStream.h"
#include "MemoryStateFile.h"
#include "zip/ZipArchiveWriter.h"
#include "Config.h"
#include "Profiler.h"
#include "iop/IopBios.h"
#include "iop/DirectoryDevice.h"
#include "iop/IsoDevice.h"

#define STATE_EE        ("ee")
#define STATE_VU1       ("vu1")
#define STATE_RAM       ("ram")
#define STATE_SPR       ("spr")
#define STATE_VUMEM1    ("vumem1")
#define STATE_MICROMEM1 ("micromem1")

#define PREF_PS2_HOST_DIRECTORY             ("ps2.host.directory")
#define PREF_PS2_MC0_DIRECTORY              ("ps2.mc0.directory")
#define PREF_PS2_MC1_DIRECTORY              ("ps2.mc1.directory")
#define PREF_PS2_HOST_DIRECTORY_DEFAULT     ("./vfs/host")
#define PREF_PS2_MC0_DIRECTORY_DEFAULT      ("./vfs/mc0")
#define PREF_PS2_MC1_DIRECTORY_DEFAULT      ("./vfs/mc1")

#ifdef _DEBUG

//#define		SCREENTICKS		10000
//#define		VBLANKTICKS		1000
//#define		SCREENTICKS		500000
#define		SCREENTICKS		1000000
#define		VBLANKTICKS		10000

#else

#define		SCREENTICKS		2000000
//#define		SCREENTICKS		4833333
//#define		SCREENTICKS		2000000
//#define		SCREENTICKS		1000000
#define		VBLANKTICKS		10000

#endif

using namespace Framework;
using namespace boost;
using namespace std;
using namespace std::tr1;

CPS2VM::CPS2VM() :
m_pRAM(new uint8[PS2::EERAMSIZE]),
m_pBIOS(new uint8[PS2::EEBIOSSIZE]),
m_pSPR(new uint8[PS2::SPRSIZE]),
m_iopRam(new uint8[PS2::IOPRAMSIZE]),
m_pVUMem0(NULL),
m_pMicroMem0(NULL),
m_pVUMem1(new uint8[PS2::VUMEM1SIZE]),
m_pMicroMem1(new uint8[PS2::MICROMEM1SIZE]),
m_pThread(NULL),
m_EE(MEMORYMAP_ENDIAN_LSBF, 0x00000000, 0x20000000),
m_VU1(MEMORYMAP_ENDIAN_LSBF, 0x00000000, 0x00008000),
m_iop(MEMORYMAP_ENDIAN_LSBF, 0x00000000, 0x00400000),
m_executor(m_EE),
m_nStatus(PAUSED),
m_nEnd(false),
m_pGS(NULL),
m_pPad(NULL),
m_singleStep(false),
m_nVBlankTicks(SCREENTICKS),
m_nInVBlank(false),
m_pCDROM0(NULL),
m_dmac(m_pRAM, m_pSPR),
m_gif(m_pGS, m_pRAM, m_pSPR),
m_sif(m_dmac, m_pRAM),
m_vif(m_gif, m_pRAM, CVIF::VPUINIT(m_pMicroMem0, m_pVUMem0, NULL), CVIF::VPUINIT(m_pMicroMem1, m_pVUMem1, &m_VU1)),
m_intc(m_dmac)
{
	CConfig::GetInstance().RegisterPreferenceString(PREF_PS2_HOST_DIRECTORY, PREF_PS2_HOST_DIRECTORY_DEFAULT);
	CConfig::GetInstance().RegisterPreferenceString(PREF_PS2_MC0_DIRECTORY, PREF_PS2_MC0_DIRECTORY_DEFAULT);
	CConfig::GetInstance().RegisterPreferenceString(PREF_PS2_MC1_DIRECTORY, PREF_PS2_MC1_DIRECTORY_DEFAULT);

    m_iopOs = new CIopBios(0x100, m_iop, m_iopRam, PS2::IOPRAMSIZE, m_sif, m_pCDROM0);
    m_os = new CPS2OS(m_EE, m_VU1, m_pRAM, m_pBIOS, m_pGS, m_sif, *m_iopOs);
}

CPS2VM::~CPS2VM()
{
    delete m_os;
    delete m_iopOs;
}

//////////////////////////////////////////////////
//Various Message Functions
//////////////////////////////////////////////////

void CPS2VM::CreateGSHandler(const CGSHandler::FactoryFunction& factoryFunction)
{
	if(m_pGS != NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::CreateGsImpl, this, factoryFunction), true);
}

CGSHandler* CPS2VM::GetGSHandler()
{
	return m_pGS;
}

void CPS2VM::DestroyGSHandler()
{
	if(m_pGS == NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::DestroyGsImpl, this), true);
}

void CPS2VM::CreatePadHandler(const CPadHandler::FactoryFunction& factoryFunction)
{
    if(m_pPad != NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::CreatePadHandlerImpl, this, factoryFunction), true);
}

void CPS2VM::DestroyPadHandler()
{
	if(m_pPad == NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::DestroyPadHandlerImpl, this), true);
}

CVirtualMachine::STATUS CPS2VM::GetStatus() const
{
    return m_nStatus;
}

void CPS2VM::Step()
{
    if(GetStatus() == RUNNING) return;
    m_singleStep = true;
    m_mailBox.SendCall(bind(&CPS2VM::ResumeImpl, this), true);
}

void CPS2VM::Resume()
{
    if(m_nStatus == RUNNING) return;
    m_mailBox.SendCall(bind(&CPS2VM::ResumeImpl, this), true);
    m_OnRunningStateChange();
}

void CPS2VM::Pause()
{
	if(m_nStatus == PAUSED) return;
    m_mailBox.SendCall(bind(&CPS2VM::PauseImpl, this), true);
    m_OnMachineStateChange();
    m_OnRunningStateChange();
}

void CPS2VM::Reset()
{
    assert(m_nStatus == PAUSED);
    ResetVM();
}

void CPS2VM::DumpEEThreadSchedule()
{
//	if(m_pOS == NULL) return;
	if(m_nStatus != PAUSED) return;
	m_os->DumpThreadSchedule();
}

void CPS2VM::DumpEEIntcHandlers()
{
//	if(m_pOS == NULL) return;
	if(m_nStatus != PAUSED) return;
	m_os->DumpIntcHandlers();
}

void CPS2VM::DumpEEDmacHandlers()
{
//	if(m_pOS == NULL) return;
	if(m_nStatus != PAUSED) return;
	m_os->DumpDmacHandlers();
}

void CPS2VM::Initialize()
{
	CreateVM();
    m_pThread = new thread(bind(&CPS2VM::EmuThread, this));
}

void CPS2VM::Destroy()
{
    m_mailBox.SendCall(bind(&CPS2VM::DestroyImpl, this));
	m_pThread->join();
	DELETEPTR(m_pThread);
	DestroyVM();
}

unsigned int CPS2VM::SaveState(const char* sPath)
{
    unsigned int result = 0;
    m_mailBox.SendCall(bind(&CPS2VM::SaveVMState, this, sPath, ref(result)), true);
    return result;
}

unsigned int CPS2VM::LoadState(const char* sPath)
{
    unsigned int result = 0;
    m_mailBox.SendCall(bind(&CPS2VM::LoadVMState, this, sPath, ref(result)), true);
    return result;
}

//unsigned int CPS2VM::SendMessage(PS2VM_MSG nMsg, void* pParam)
//{
//	return m_MsgBox.SendMessage(nMsg, pParam);
//}

//////////////////////////////////////////////////
//Non extern callable methods
//////////////////////////////////////////////////

void CPS2VM::CreateVM()
{
    printf("PS2VM: Virtual Machine Memory Usage: RAM: %i MBs, BIOS: %i MBs, SPR: %i KBs.\r\n", 
        PS2::EERAMSIZE / 0x100000, PS2::EEBIOSSIZE / 0x100000, PS2::SPRSIZE / 0x1000);
	
	//EmotionEngine context setup
	m_EE.m_pMemoryMap->InsertReadMap(0x00000000, 0x01FFFFFF, m_pRAM,				                        0x00);
	m_EE.m_pMemoryMap->InsertReadMap(0x02000000, 0x02003FFF, m_pSPR,				                        0x01);
    m_EE.m_pMemoryMap->InsertReadMap(0x10000000, 0x10FFFFFF, bind(&CPS2VM::IOPortReadHandler, this, _1),    0x02);
    m_EE.m_pMemoryMap->InsertReadMap(0x12000000, 0x12FFFFFF, bind(&CPS2VM::IOPortReadHandler, this, _1),    0x03);
	m_EE.m_pMemoryMap->InsertReadMap(0x1FC00000, 0x1FFFFFFF, m_pBIOS,				                        0x04);

	m_EE.m_pMemoryMap->InsertWriteMap(0x00000000, 0x01FFFFFF, m_pRAM,				                            0x00);
	m_EE.m_pMemoryMap->InsertWriteMap(0x02000000, 0x02003FFF, m_pSPR,				                            0x01);
    m_EE.m_pMemoryMap->InsertWriteMap(0x10000000, 0x10FFFFFF, bind(&CPS2VM::IOPortWriteHandler, this, _1, _2),	0x02);
    m_EE.m_pMemoryMap->InsertWriteMap(0x12000000, 0x12FFFFFF, bind(&CPS2VM::IOPortWriteHandler,	this, _1, _2),  0x03);

    m_EE.m_pMemoryMap->SetWriteNotifyHandler(bind(&CPS2VM::EEMemWriteHandler, this, _1));

	m_EE.m_pArch			= &g_MAEE;
	m_EE.m_pCOP[0]			= &g_COPSCU;
	m_EE.m_pCOP[1]			= &g_COPFPU;
	m_EE.m_pCOP[2]			= &g_COPVU;

    m_EE.m_handlerParam     = this;
    m_EE.m_pAddrTranslator	= CPS2OS::TranslateAddress;
    m_EE.m_pSysCallHandler  = EESysCallHandlerStub;
#ifdef DEBUGGER_INCLUDED
    m_EE.m_pTickFunction	= EETickFunctionStub;
#else
	m_EE.m_pTickFunction	= NULL;
#endif

	//Vector Unit 1 context setup
	m_VU1.m_pMemoryMap->InsertReadMap(0x00000000, 0x00003FFF, m_pVUMem1,	0x00);
	m_VU1.m_pMemoryMap->InsertReadMap(0x00004000, 0x00007FFF, m_pMicroMem1,	0x01);

	m_VU1.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00003FFF, m_pVUMem1,	0x00);

	m_VU1.m_pArch			= &g_MAVU;
	m_VU1.m_pAddrTranslator	= CMIPS::TranslateAddress64;

#ifdef DEBUGGER_INCLUDED
	m_VU1.m_pTickFunction	= VU1TickFunctionStub;
#else
	m_VU1.m_pTickFunction	= NULL;
#endif

    m_dmac.SetChannelTransferFunction(1, bind(&CVIF::ReceiveDMA1, &m_vif, _1, _2, _4));
    m_dmac.SetChannelTransferFunction(2, bind(&CGIF::ReceiveDMA, &m_gif, _1, _2, _3, _4));
    m_dmac.SetChannelTransferFunction(4, bind(&CIPU::ReceiveDMA4, &m_ipu, _1, _2, _4, m_pRAM));
    m_dmac.SetChannelTransferFunction(5, bind(&CSIF::ReceiveDMA5, &m_sif, _1, _2, _3, _4));
    m_dmac.SetChannelTransferFunction(6, bind(&CSIF::ReceiveDMA6, &m_sif, _1, _2, _3, _4));

    m_ipu.SetDMA3ReceiveHandler(bind(&CDMAC::ResumeDMA3, &m_dmac, _1, _2));

	CDROM0_Initialize();

	ResetVM();

	printf("PS2VM: Created PS2 Virtual Machine.\r\n");
}

void CPS2VM::ResetVM()
{
    m_executor.Clear();

    memset(m_pRAM,			0, PS2::EERAMSIZE);
    memset(m_pSPR,			0, PS2::SPRSIZE);
    memset(m_pVUMem1,		0, PS2::VUMEM1SIZE);
    memset(m_pMicroMem1,	0, PS2::MICROMEM1SIZE);

	//LoadBIOS();

	//Reset Context
	memset(&m_EE.m_State, 0, sizeof(MIPSSTATE));
	memset(&m_VU1.m_State, 0, sizeof(MIPSSTATE));
    m_EE.m_State.nDelayedJumpAddr = MIPS_INVALID_PC;
    m_VU1.m_State.nDelayedJumpAddr = MIPS_INVALID_PC;

	//Set VF0[w] to 1.0
	m_EE.m_State.nCOP2[0].nV3	= 0x3F800000;
	m_VU1.m_State.nCOP2[0].nV3	= 0x3F800000;

    m_VU1.m_State.nPC		= 0x4000;

	m_nStatus = PAUSED;
	
	//Reset subunits
	CDROM0_Reset();
    m_sif.Reset();
    m_ipu.Reset();
    m_gif.Reset();
    m_vif.Reset();
    m_dmac.Reset();
	m_intc.Reset();
//	CTimer::Reset();

	if(m_pGS != NULL)
	{
		m_pGS->Reset();
	}

//	DELETEPTR(m_pOS);
//	m_pOS = new CPS2OS(m_EE, m_VU1, m_pRAM, m_pBIOS, m_pGS);
    m_os->Release();
    m_os->Initialize();
    m_iopOs->Reset();

    m_iopOs->GetIoman()->RegisterDevice("host", new Iop::Ioman::CDirectoryDevice(PREF_PS2_HOST_DIRECTORY));
    m_iopOs->GetIoman()->RegisterDevice("mc0", new Iop::Ioman::CDirectoryDevice(PREF_PS2_MC0_DIRECTORY));
    m_iopOs->GetIoman()->RegisterDevice("mc1", new Iop::Ioman::CDirectoryDevice(PREF_PS2_MC1_DIRECTORY));
    m_iopOs->GetIoman()->RegisterDevice("cdrom0", new Iop::Ioman::CIsoDevice(m_pCDROM0));

    RegisterModulesInPadHandler();
}

void CPS2VM::DestroyVM()
{
	CDROM0_Destroy();

	FREEPTR(m_pRAM);
	FREEPTR(m_pBIOS);
}

void CPS2VM::SaveVMState(const char* sPath, unsigned int& result)
{
	if(m_pGS == NULL)
	{
		printf("PS2VM: GS Handler was not instancied. Cannot save state.\r\n");
        result = 1;
        return;
	}
    
    try
    {
        CStdStream stateStream(sPath, "wb");
        CZipArchiveWriter archive;

        archive.InsertFile(new CMemoryStateFile(STATE_EE,           &m_EE.m_State,  sizeof(MIPSSTATE)));
        archive.InsertFile(new CMemoryStateFile(STATE_VU1,          &m_VU1.m_State, sizeof(MIPSSTATE)));
        archive.InsertFile(new CMemoryStateFile(STATE_RAM,          m_pRAM,         PS2::EERAMSIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_SPR,          m_pSPR,         PS2::SPRSIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_VUMEM1,       m_pVUMem1,      PS2::VUMEM1SIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_MICROMEM1,    m_pMicroMem1,   PS2::MICROMEM1SIZE));

        m_pGS->SaveState(archive);
        m_dmac.SaveState(archive);
        m_intc.SaveState(archive);
        m_sif.SaveState(archive);
        m_iopOs->GetDbcman()->SaveState(archive);
        m_iopOs->GetPadman()->SaveState(archive);

//	CVIF::SaveState(pS);

        archive.Write(stateStream);
    }
    catch(...)
    {
        result = 1;
        return;
    }

    printf("PS2VM: Saved state to file '%s'.\r\n", sPath);

    result = 0;
}

void CPS2VM::LoadVMState(const char* sPath, unsigned int& result)
{
	if(m_pGS == NULL)
	{
		printf("PS2VM: GS Handler was not instancied. Cannot load state.\r\n");
        result = 1;
        return;
	}

    try
    {
        CStdStream stateStream(sPath, "rb");
        CZipArchiveReader archive(stateStream);

        archive.BeginReadFile(STATE_EE          )->Read(&m_EE.m_State,  sizeof(MIPSSTATE));
        archive.BeginReadFile(STATE_VU1         )->Read(&m_VU1.m_State, sizeof(MIPSSTATE));
        archive.BeginReadFile(STATE_RAM         )->Read(m_pRAM,         PS2::EERAMSIZE);
        archive.BeginReadFile(STATE_SPR         )->Read(m_pSPR,         PS2::SPRSIZE);
        archive.BeginReadFile(STATE_VUMEM1      )->Read(m_pVUMem1,      PS2::VUMEM1SIZE);
        archive.BeginReadFile(STATE_MICROMEM1   )->Read(m_pMicroMem1,   PS2::MICROMEM1SIZE);

        m_pGS->LoadState(archive);
        m_dmac.LoadState(archive);
        m_intc.LoadState(archive);
        m_sif.LoadState(archive);
        m_iopOs->GetDbcman()->LoadState(archive);
        m_iopOs->GetPadman()->LoadState(archive);
//	CVIF::LoadState(pS);

    }
    catch(...)
    {
        result = 1;
        return;
    }

    printf("PS2VM: Loaded state from file '%s'.\r\n", sPath);

    m_executor.Clear();
	m_OnMachineStateChange();

    result = 0;
}

void CPS2VM::PauseImpl()
{
    m_nStatus = PAUSED;
//    printf("PS2VM: Virtual Machine paused.\r\n");
}

void CPS2VM::ResumeImpl()
{
    m_nStatus = RUNNING;
//    printf("PS2VM: Virtual Machine started.\r\n");
}

void CPS2VM::DestroyImpl()
{
    DELETEPTR(m_pGS);
    m_nEnd = true;
}

void CPS2VM::CreateGsImpl(const CGSHandler::FactoryFunction& factoryFunction)
{
    m_pGS = factoryFunction();
    m_pGS->Initialize();
}

void CPS2VM::DestroyGsImpl()
{
    DELETEPTR(m_pGS);
}

void CPS2VM::CreatePadHandlerImpl(const CPadHandler::FactoryFunction& factoryFunction)
{
    m_pPad = factoryFunction();
    RegisterModulesInPadHandler();
}

void CPS2VM::DestroyPadHandlerImpl()
{
    DELETEPTR(m_pPad);
}

void CPS2VM::CDROM0_Initialize()
{
	CConfig::GetInstance().RegisterPreferenceString("ps2.cdrom0.path", "");
	m_pCDROM0 = NULL;
}

void CPS2VM::CDROM0_Reset()
{
	DELETEPTR(m_pCDROM0);
	CDROM0_Mount(CConfig::GetInstance().GetPreferenceString("ps2.cdrom0.path"));
}

void CPS2VM::CDROM0_Mount(const char* sPath)
{
	//Check if there's an m_pCDROM0 already
	//Check if files are linked to this m_pCDROM0 too and do something with them

    size_t pathLength = strlen(sPath);
	if(pathLength != 0)
	{
		try
		{
	        CStream* pStream(NULL);
            const char* extension = "";
            if(pathLength >= 4)
            {
                extension = sPath + pathLength - 4;
            }

			//Gotta think of something better than that...
            if(!stricmp(extension, ".isz"))
            {
                pStream = new CIszImageStream(new CStdStream(sPath, "rb"));
            }
#ifdef WIN32
            else if(sPath[0] == '\\')
            {
	            pStream = new Win32::CVolumeStream(sPath[4]);
            }
#else
            else
            {
			    pStream = new Posix::CVolumeStream(sPath);
            }
#endif

            //If it's null after all that, just feed it to a StdStream
            if(pStream == NULL)
            {
                pStream = new CStdStream(sPath, "rb");
            }

			m_pCDROM0 = new CISO9660(pStream);
		}
		catch(const exception& Exception)
		{
			printf("PS2VM: Error mounting cdrom0 device: %s\r\n", Exception.what());
		}
	}

	CConfig::GetInstance().SetPreferenceString("ps2.cdrom0.path", sPath);
}

void CPS2VM::CDROM0_Destroy()
{
	DELETEPTR(m_pCDROM0);
}

void CPS2VM::LoadBIOS()
{
	CStdStream BiosStream(fopen("./vfs/rom0/scph10000.bin", "rb"));
    BiosStream.Read(m_pBIOS, PS2::EEBIOSSIZE);
}

void CPS2VM::RegisterModulesInPadHandler()
{
	if(m_pPad == NULL) return;

	m_pPad->RemoveAllListeners();
	m_pPad->InsertListener(m_iopOs->GetDbcman());
    m_pPad->InsertListener(m_iopOs->GetPadman());
}

uint32 CPS2VM::IOPortReadHandler(uint32 nAddress)
{
	uint32 nReturn;

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	nReturn = 0;
	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
//		nReturn = CTimer::GetRegister(nAddress);
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
        nReturn = m_ipu.GetRegister(nAddress);
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		nReturn = m_dmac.GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		nReturn = m_intc.GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		nReturn = m_dmac.GetRegister(nAddress);
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_pGS != NULL)
		{
			nReturn = m_pGS->ReadPrivRegister(nAddress);		
		}
	}
	else
	{
		printf("PS2VM: Read an unhandled IO port (0x%0.8X).\r\n", nAddress);
	}

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif

	return nReturn;
}

uint32 CPS2VM::IOPortWriteHandler(uint32 nAddress, uint32 nData)
{
#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
//		CTimer::SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
        m_ipu.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10007000 && nAddress <= 0x1000702F)
	{
        m_ipu.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		m_dmac.SetRegister(nAddress, nData);
    }
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		m_intc.SetRegister(nAddress, nData);
	}
	else if(nAddress == 0x1000F180)
	{
		//stdout data
        throw runtime_error("Not implemented.");
//        m_sif.GetFileIO()->Write(1, 1, &nData);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		m_dmac.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_pGS != NULL)
		{
			m_pGS->WritePrivRegister(nAddress, nData);
		}
	}
	else
	{
		printf("PS2VM: Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, nData, m_EE.m_State.nPC);
	}

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif

    return 0;
}

void CPS2VM::EEMemWriteHandler(uint32 nAddress)
{
    if(nAddress < PS2::EERAMSIZE)
	{
		//Check if the block we're about to invalidate is the same
		//as the one we're executing in

//		CCacheBlock* pBlock;
//		pBlock = m_EE.m_pExecMap->FindBlock(nAddress);
//		if(m_EE.m_pExecMap->FindBlock(m_EE.m_State.nPC) != pBlock)
//		{
//			m_EE.m_pExecMap->InvalidateBlock(nAddress);
//		}
//		else
//		{
#ifdef _DEBUG
//			printf("PS2VM: Warning. Writing to the same cache block as the one we're currently executing in. PC: 0x%0.8X\r\n",
//				m_EE.m_State.nPC);
#endif
//		}
	}
}

void CPS2VM::EESysCallHandlerStub(CMIPS* context)
{
//    CPS2VM& vm = *reinterpret_cast<CPS2VM*>(context->m_handlerParam);
//    vm.m_os.SysCallHandler();
}

unsigned int CPS2VM::EETickFunctionStub(unsigned int ticks, CMIPS* context)
{
    return reinterpret_cast<CPS2VM*>(context->m_handlerParam)->EETickFunction(ticks);
}

unsigned int CPS2VM::VU1TickFunctionStub(unsigned int ticks, CMIPS* context)
{
    return reinterpret_cast<CPS2VM*>(context->m_handlerParam)->VU1TickFunction(ticks);
}

unsigned int CPS2VM::EETickFunction(unsigned int nTicks)
{

#ifdef DEBUGGER_INCLUDED

	if(m_EE.m_nIllOpcode != MIPS_INVALID_PC)
	{
		printf("PS2VM: (EmotionEngine) Illegal opcode encountered at 0x%0.8X.\r\n", m_EE.m_nIllOpcode);
		m_EE.m_nIllOpcode = MIPS_INVALID_PC;
		assert(0);
	}

	if(m_EE.MustBreak())
	{
		return 1;
	}

	//TODO: Check if we can remove this if there's no debugger around
//	if(m_MsgBox.IsMessagePending())
    if(m_mailBox.IsPending())
	{
		return 1;
	}

#endif

	return 0;
}

unsigned int CPS2VM::VU1TickFunction(unsigned int nTicks)
{
#ifdef DEBUGGER_INCLUDED

	if(m_VU1.m_nIllOpcode != MIPS_INVALID_PC)
	{
		printf("PS2VM: (VectorUnit1) Illegal/unimplemented instruction encountered at 0x%0.8X.\r\n", m_VU1.m_nIllOpcode);
		m_VU1.m_nIllOpcode = MIPS_INVALID_PC;
	}

	if(m_VU1.MustBreak())
	{
		return 1;
	}

	//TODO: Check if we can remove this if there's no debugger around
//	if(m_MsgBox.IsMessagePending())
    if(m_mailBox.IsPending())
    {
		return 1;
	}

#endif

	return 0;
}

void CPS2VM::EmuThread()
{
    //BEGIN: Frame limiter init
//    unsigned int lastFrameCount = 0;
//    xtime lastFrameTime;
//    xtime_get(&lastFrameTime, boost::TIME_UTC);
    //END
	m_nEnd = false;
	while(1)
	{
        while(m_mailBox.IsPending())
        {
            m_mailBox.ReceiveCall();
        }
		if(m_nEnd) break;
		if(m_nStatus == PAUSED)
		{
            //Sleep during 100ms
            xtime xt;
            xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 100 * 1000000;
			thread::sleep(xt);
		}
		if(m_nStatus == RUNNING)
        {
            //Synchonization methods
            //1984.elf - CSR = CSR & 0x08; while(CSR & 0x08 == 0);
			if(static_cast<int>(m_nVBlankTicks) <= 0)
			{
				m_nInVBlank = !m_nInVBlank;
				if(m_nInVBlank)
				{
					m_nVBlankTicks += VBLANKTICKS;
                    m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_START);
                    if(m_pGS != NULL)
                    {
					    m_pGS->SetVBlank();
                    }


					//Old Flipping Method
					//m_pGS->Flip();
					//m_OnNewFrame();
                    //////

                    if(m_pPad != NULL)
                    {
                        m_pPad->Update(m_pRAM);
                    }
				}
				else
				{
					m_nVBlankTicks += SCREENTICKS;
                    m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_END);
					if(m_pGS != NULL)
					{
						m_pGS->ResetVBlank();
					}
				}
			}
            m_dmac.ResumeDMA4();
            if(!m_EE.m_State.nHasException)
            {
                if(m_intc.IsInterruptPending())
                {
                    m_os->ExceptionHandler();
                    //Do we need to do some special treatment when we're serving an interrupt?
                    //m_EE.m_State.nHasException = 1;
                }
            }
            if(!m_EE.m_State.nHasException)
            {
                int executeQuota = m_singleStep ? 1 : 5000;
				m_nVBlankTicks -= (executeQuota - m_executor.Execute(executeQuota));
            }
            if(m_EE.m_State.nHasException)
            {
                m_os->SysCallHandler();
                assert(!m_EE.m_State.nHasException);
            }
            {
                CBasicBlock* nextBlock = m_executor.FindBlockAt(m_EE.m_State.nPC);
                if(nextBlock != NULL && nextBlock->GetSelfLoopCount() > 5000)
                {
                    m_nVBlankTicks = 0;
                }
            }
            //BEGIN: Frame limiter
//            if(m_pGS != NULL && lastFrameCount != m_pGS->GetFrameCount())
//            {
//                xtime currentTime;
//                xtime_get(&currentTime, boost::TIME_UTC);
//                xtime::xtime_nsec_t timeDiff = currentTime.nsec - lastFrameTime.nsec;
//                if((currentTime.sec == lastFrameTime.sec) && (timeDiff < (1000000 * 8)))
//                {
//                    currentTime.nsec += (1000000 * 8) - timeDiff;
//                    thread::sleep(currentTime);
//                }
//                lastFrameCount = m_pGS->GetFrameCount();
//                xtime_get(&lastFrameTime, boost::TIME_UTC);
//            }
            //END
#ifdef _DEBUG
            if(m_vif.IsPauseNeeded())
            {
                //Force pause
                m_vif.SetPauseNeeded(false);
                m_singleStep = true;
            }
#endif
            if(m_executor.MustBreak() || m_singleStep)
            {
                m_nStatus = PAUSED;
                m_singleStep = false;
                m_OnRunningStateChange();
                m_OnMachineStateChange();
            }
		}
	}
}
