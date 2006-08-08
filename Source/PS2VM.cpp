#include <stdio.h>
#include <exception>
#include "PS2VM.h"
#include "DMAC.h"
#include "INTC.h"
#include "IPU.h"
#include "GIF.h"
#include "SIF.h"
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
#include "VolumeStream.h"
#include "Config.h"
#include "Profiler.h"

#ifdef _DEBUG

//#define		SCREENTICKS		10000
//#define		VBLANKTICKS		1000
//#define		SCREENTICKS		500000
#define		SCREENTICKS		1000000
#define		VBLANKTICKS		10000

#else

//#define		SCREENTICKS		4833333
#define		SCREENTICKS		2000000
//#define		SCREENTICKS		1000000
#define		VBLANKTICKS		100000

#endif

using namespace Framework;
using namespace boost;
using namespace std;

uint8*			CPS2VM::m_pRAM						= NULL;
uint8*			CPS2VM::m_pBIOS						= NULL;
uint8*			CPS2VM::m_pSPR						= NULL;
uint8*			CPS2VM::m_pVUMem0					= NULL;
uint8*			CPS2VM::m_pMicroMem0				= NULL;
uint8*			CPS2VM::m_pVUMem1					= NULL;
uint8*			CPS2VM::m_pMicroMem1				= NULL;
thread*			CPS2VM::m_pThread					= NULL;
CMIPS			CPS2VM::m_EE(MEMORYMAP_ENDIAN_LSBF,		0x00000000, 0x20000000);
CMIPS			CPS2VM::m_VU1(MEMORYMAP_ENDIAN_LSBF,	0x00000000, 0x00008000);
CThreadMsg		CPS2VM::m_MsgBox;
PS2VM_STATUS	CPS2VM::m_nStatus					= PS2VM_STATUS_PAUSED;
CGSHandler*		CPS2VM::m_pGS						= NULL;
CPadHandler*	CPS2VM::m_pPad						= NULL;
unsigned int	CPS2VM::m_nVBlankTicks				= SCREENTICKS;
bool			CPS2VM::m_nInVBlank					= false;
CEvent<int>		CPS2VM::m_OnMachineStateChange;
CEvent<int>		CPS2VM::m_OnRunningStateChange;
CEvent<int>		CPS2VM::m_OnNewFrame;
CLogControl		CPS2VM::m_Logging;
CPS2OS*			CPS2VM::m_pOS						= NULL;
CISO9660*		CPS2VM::m_pCDROM0					= NULL;

//////////////////////////////////////////////////
//Various Message Functions
//////////////////////////////////////////////////

void CPS2VM::CreateGSHandler(GSHANDLERFACTORY pF, void* pParam)
{
	CREATEGSHANDLERPARAM Param;
	if(m_pGS != NULL) return;

	Param.pFactory	= pF;
	Param.pParam	= pParam;

	SendMessage(PS2VM_MSG_CREATEGS, &Param);
}

CGSHandler* CPS2VM::GetGSHandler()
{
	return m_pGS;
}

void CPS2VM::DestroyGSHandler()
{
	if(m_pGS == NULL) return;
	SendMessage(PS2VM_MSG_DESTROYGS);
}

void CPS2VM::CreatePadHandler(PADHANDLERFACTORY pF, void* pParam)
{
	CREATEPADHANDLERPARAM Param;
	if(m_pPad != NULL) return;

	Param.pFactory	= pF;
	Param.pParam	= pParam;

	SendMessage(PS2VM_MSG_CREATEPAD, &Param);
}

void CPS2VM::DestroyPadHandler()
{
	if(m_pPad == NULL) return;
	SendMessage(PS2VM_MSG_DESTROYPAD);
}

void CPS2VM::Resume()
{
	if(m_nStatus == PS2VM_STATUS_RUNNING) return;
	SendMessage(PS2VM_MSG_RESUME);
}

void CPS2VM::Pause()
{
	if(m_nStatus == PS2VM_STATUS_PAUSED) return;
	SendMessage(PS2VM_MSG_PAUSE);
}

void CPS2VM::Reset()
{
	SendMessage(PS2VM_MSG_RESET);
}

void CPS2VM::DumpEEThreadSchedule()
{
	if(m_pOS == NULL) return;
	if(m_nStatus != PS2VM_STATUS_PAUSED) return;
	m_pOS->DumpThreadSchedule();
}

void CPS2VM::DumpEEIntcHandlers()
{
	if(m_pOS == NULL) return;
	if(m_nStatus != PS2VM_STATUS_PAUSED) return;
	m_pOS->DumpIntcHandlers();
}

void CPS2VM::DumpEEDmacHandlers()
{
	if(m_pOS == NULL) return;
	if(m_nStatus != PS2VM_STATUS_PAUSED) return;
	m_pOS->DumpDmacHandlers();
}

void CPS2VM::Initialize()
{
	CreateVM();
	m_pThread = new thread(&EmuThread);
}

void CPS2VM::Destroy()
{
	SendMessage(PS2VM_MSG_DESTROY);
	m_pThread->join();
	DELETEPTR(m_pThread);
	DestroyVM();
}

unsigned int CPS2VM::SaveState(const char* sPath)
{
	return SendMessage(PS2VM_MSG_SAVESTATE, (void*)sPath);
}

unsigned int CPS2VM::LoadState(const char* sPath)
{
	return SendMessage(PS2VM_MSG_LOADSTATE, (void*)sPath);
}

unsigned int CPS2VM::SendMessage(PS2VM_MSG nMsg, void* pParam)
{
	return m_MsgBox.SendMessage(nMsg, pParam);
}

//////////////////////////////////////////////////
//Non extern callable methods
//////////////////////////////////////////////////

void CPS2VM::CreateVM()
{
	printf("PS2VM: Virtual Machine Memory Usage: RAM: %i MBs, BIOS: %i MBs, SPR: %i KBs.\r\n", RAMSIZE / 0x100000, BIOSSIZE / 0x100000, SPRSIZE / 0x1000);
	
	m_pRAM			= (uint8*)malloc(RAMSIZE);
	m_pBIOS			= (uint8*)malloc(BIOSSIZE);
	m_pSPR			= (uint8*)malloc(SPRSIZE);
	m_pVUMem1		= (uint8*)malloc(VUMEM1SIZE);
	m_pMicroMem1	= (uint8*)malloc(MICROMEM1SIZE);

	//EmotionEngine context setup
	m_EE.m_pMemoryMap->InsertReadMap(0x00000000, 0x01FFFFFF, m_pRAM,				MEMORYMAP_TYPE_MEMORY,		0x00);
	m_EE.m_pMemoryMap->InsertReadMap(0x02000000, 0x02003FFF, m_pSPR,				MEMORYMAP_TYPE_MEMORY,		0x01);
	m_EE.m_pMemoryMap->InsertReadMap(0x10000000, 0x10FFFFFF, IOPortReadHandler,		MEMORYMAP_TYPE_FUNCTION,	0x02);
	m_EE.m_pMemoryMap->InsertReadMap(0x12000000, 0x12FFFFFF, IOPortReadHandler,		MEMORYMAP_TYPE_FUNCTION,	0x03);
	m_EE.m_pMemoryMap->InsertReadMap(0x1FC00000, 0x1FFFFFFF, m_pBIOS,				MEMORYMAP_TYPE_MEMORY,		0x04);

	m_EE.m_pMemoryMap->InsertWriteMap(0x00000000, 0x01FFFFFF, m_pRAM,				MEMORYMAP_TYPE_MEMORY,		0x00);
	m_EE.m_pMemoryMap->InsertWriteMap(0x02000000, 0x02003FFF, m_pSPR,				MEMORYMAP_TYPE_MEMORY,		0x01);
	m_EE.m_pMemoryMap->InsertWriteMap(0x10000000, 0x10FFFFFF, IOPortWriteHandler,	MEMORYMAP_TYPE_FUNCTION,	0x02);
	m_EE.m_pMemoryMap->InsertWriteMap(0x12000000, 0x12FFFFFF, IOPortWriteHandler,	MEMORYMAP_TYPE_FUNCTION,	0x03);

	m_EE.m_pArch			= &g_MAEE;
	m_EE.m_pCOP[0]			= &g_COPSCU;
	m_EE.m_pCOP[1]			= &g_COPFPU;
	m_EE.m_pCOP[2]			= &g_COPVU;

	m_EE.m_pAddrTranslator	= CPS2OS::TranslateAddress;

#ifdef DEBUGGER_INCLUDED
	m_EE.m_pTickFunction	= EETickFunction;
#else
	m_EE.m_pTickFunction	= NULL;
#endif

	//Vector Unit 1 context setup
	m_VU1.m_pMemoryMap->InsertReadMap(0x00000000, 0x00003FFF, m_pVUMem1,	MEMORYMAP_TYPE_MEMORY,			0x00);
	m_VU1.m_pMemoryMap->InsertReadMap(0x00004000, 0x00007FFF, m_pMicroMem1,	MEMORYMAP_TYPE_MEMORY,			0x01);

	m_VU1.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00003FFF, m_pVUMem1,	MEMORYMAP_TYPE_MEMORY,			0x00);

	m_VU1.m_pArch			= &g_MAVU;
	m_VU1.m_pAddrTranslator	= CMIPS::TranslateAddress64;

#ifdef DEBUGGER_INCLUDED
	m_VU1.m_pTickFunction	= VU1TickFunction;
#else
	m_VU1.m_pTickFunction	= NULL;
#endif

	CDROM0_Initialize();

	ResetVM();

	printf("PS2VM: Created PS2 Virtual Machine.\r\n");
}

void CPS2VM::ResetVM()
{
	memset(m_pRAM,			0, RAMSIZE);
	memset(m_pSPR,			0, SPRSIZE);
	memset(m_pVUMem1,		0, VUMEM1SIZE);
	memset(m_pMicroMem1,	0, MICROMEM1SIZE);

	//LoadBIOS();

	//Reset Context
	memset(&m_EE.m_State, 0, sizeof(MIPSSTATE));
	memset(&m_VU1.m_State, 0, sizeof(MIPSSTATE));

	//Set VF0[w] to 1.0
	m_EE.m_State.nCOP2[0].nV3	= 0x3F800000;
	m_VU1.m_State.nCOP2[0].nV3	= 0x3F800000;

	m_VU1.m_State.nPC		= 0x4000;

	m_nStatus = PS2VM_STATUS_PAUSED;
	
	//Reset subunits
	CDROM0_Reset();
	CSIF::Reset();
	CIPU::Reset();
	CGIF::Reset();
	CVIF::Reset();
	CDMAC::Reset();
	CINTC::Reset();
	CTimer::Reset();

	if(m_pGS != NULL)
	{
		m_pGS->Reset();
	}

	DELETEPTR(m_pOS);
	m_pOS = new CPS2OS;

	RegisterModulesInPadHandler();
}

void CPS2VM::DestroyVM()
{
	CDROM0_Destroy();

	FREEPTR(m_pRAM);
	FREEPTR(m_pBIOS);
	DELETEPTR(m_pOS);
}

unsigned int CPS2VM::SaveVMState(const char* sPath)
{
	CStream* pS;

	if(m_pGS == NULL)
	{
		printf("PS2VM: GS Handler was not instancied. Cannot save state.\r\n");
		return 1;
	}

	try
	{
		pS = new CGZipStream(sPath, "wb");
	}
	catch(...)
	{
		return 1;
	}

	pS->Write(&CPS2VM::m_EE.m_State, sizeof(MIPSSTATE));
	pS->Write(&CPS2VM::m_VU1.m_State, sizeof(MIPSSTATE));

	pS->Write(CPS2VM::m_pRAM,		RAMSIZE);
	pS->Write(CPS2VM::m_pSPR,		SPRSIZE);
	pS->Write(CPS2VM::m_pVUMem1,	VUMEM1SIZE);
	pS->Write(CPS2VM::m_pMicroMem1, MICROMEM1SIZE);

	m_pGS->SaveState(pS);
	CINTC::SaveState(pS);
	CDMAC::SaveState(pS);
	CSIF::SaveState(pS);
	CVIF::SaveState(pS);

	delete pS;

	printf("PS2VM: Saved state to file '%s'.\r\n", sPath);

	return 0;
}

unsigned int CPS2VM::LoadVMState(const char* sPath)
{
	CStream* pS;

	if(m_pGS == NULL)
	{
		printf("PS2VM: GS Handler was not instancied. Cannot load state.\r\n");
		return 1;
	}

	try
	{
		pS = new CGZipStream(sPath, "rb");
	}
	catch(...)
	{
		return 1;
	}

	pS->Read(&CPS2VM::m_EE.m_State, sizeof(MIPSSTATE));
	pS->Read(&CPS2VM::m_VU1.m_State, sizeof(MIPSSTATE));

	pS->Read(CPS2VM::m_pRAM,		RAMSIZE);
	pS->Read(CPS2VM::m_pSPR,		SPRSIZE);
	pS->Read(CPS2VM::m_pVUMem1,		VUMEM1SIZE);
	pS->Read(CPS2VM::m_pMicroMem1,	MICROMEM1SIZE);

	m_pGS->LoadState(pS);
	CINTC::LoadState(pS);
	CDMAC::LoadState(pS);
	CSIF::LoadState(pS);
	CVIF::LoadState(pS);

	delete pS;

	printf("PS2VM: Loaded state from file '%s'.\r\n", sPath);

	m_OnMachineStateChange.Notify(0);

	return 0;
}

void CPS2VM::CDROM0_Initialize()
{
	CConfig::GetInstance()->RegisterPreferenceString("ps2.cdrom0.path", "");
	m_pCDROM0 = NULL;
}

void CPS2VM::CDROM0_Reset()
{
	DELETEPTR(m_pCDROM0);
	CDROM0_Mount(CConfig::GetInstance()->GetPreferenceString("ps2.cdrom0.path"));
}

void CPS2VM::CDROM0_Mount(const char* sPath)
{
	CStream* pStream;

	//Check if there's an m_pCDROM0 already
	//Check if files are linked to this m_pCDROM0 too and do something with them

	if(strlen(sPath) != 0)
	{
		try
		{
			//Gotta think of something better than that...
			if(sPath[0] == '\\')
			{
				pStream = new Win32::CVolumeStream(sPath[4]);
			}
			else
			{
				pStream = new CStdStream(fopen(sPath, "rb"));
			}
			m_pCDROM0 = new CISO9660(pStream);
		}
		catch(const exception& Exception)
		{
			printf("PS2VM: Error mounting cdrom0 device: %s\r\n", Exception.what());
		}
	}

	CConfig::GetInstance()->SetPreferenceString("ps2.cdrom0.path", sPath);
}

void CPS2VM::CDROM0_Destroy()
{
	DELETEPTR(m_pCDROM0);
}

void CPS2VM::LoadBIOS()
{
	CStdStream* pS;
	pS = new CStdStream(fopen("./vfs/rom0/scph10000.bin", "rb"));
	pS->Read(m_pBIOS, BIOSSIZE);
	delete pS;
}

void CPS2VM::RegisterModulesInPadHandler()
{
	if(m_pPad == NULL) return;

	m_pPad->RemoveAllListeners();
	m_pPad->InsertListener(CSIF::GetPadMan());
	m_pPad->InsertListener(CSIF::GetDbcMan());
}

void CPS2VM::IOPortWriteHandler(uint32 nAddress, uint32 nData)
{
#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
		CTimer::SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
		CIPU::SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10007000 && nAddress <= 0x1000702F)
	{
		CIPU::SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		CDMAC::SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		CINTC::SetRegister(nAddress, nData);
	}
	else if(nAddress == 0x1000F180)
	{
		//stdout data
		CSIF::GetFileIO()->Write(1, 1, &nData);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		CDMAC::SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		m_pGS->WritePrivRegister(nAddress, nData);
	}
	else
	{
		printf("PS2VM: Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, nData, m_EE.m_State.nPC);
	}

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif
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
		nReturn = CTimer::GetRegister(nAddress);
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
		nReturn = CIPU::GetRegister(nAddress);
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		nReturn = CDMAC::GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		nReturn = CINTC::GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		nReturn = CDMAC::GetRegister(nAddress);
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		nReturn = m_pGS->ReadPrivRegister(nAddress);
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
	if(m_MsgBox.IsMessagePending())
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
	if(m_MsgBox.IsMessagePending())
	{
		return 1;
	}

#endif

	return 0;
}

void CPS2VM::EmuThread()
{
	bool nEnd;
	CThreadMsg::MESSAGE Msg;
	unsigned int nRetValue;

	nEnd = false;

	while(1)
	{
		if(m_MsgBox.GetMessage(&Msg))
		{
			nRetValue = 0;

			switch(Msg.nMsg)
			{
			case PS2VM_MSG_PAUSE:
				m_nStatus = PS2VM_STATUS_PAUSED;
				m_OnMachineStateChange.Notify(0);
				m_OnRunningStateChange.Notify(0);
				printf("PS2VM: Virtual Machine paused.\r\n");
				break;
			case PS2VM_MSG_RESUME:
				m_nStatus = PS2VM_STATUS_RUNNING;
				m_OnRunningStateChange.Notify(0);
				printf("PS2VM: Virtual Machine started.\r\n");
				break;
			case PS2VM_MSG_DESTROY:
				DELETEPTR(m_pGS);
				nEnd = true;
				break;
			case PS2VM_MSG_CREATEGS:
				CREATEGSHANDLERPARAM* pCreateGSParam;
				pCreateGSParam = (CREATEGSHANDLERPARAM*)Msg.pParam;
				m_pGS = pCreateGSParam->pFactory(pCreateGSParam->pParam);
				break;
			case PS2VM_MSG_DESTROYGS:
				DELETEPTR(m_pGS);
				break;
			case PS2VM_MSG_CREATEPAD:
				CREATEPADHANDLERPARAM* pCreatePadParam;
				pCreatePadParam = (CREATEPADHANDLERPARAM*)Msg.pParam;
				m_pPad = pCreatePadParam->pFactory(pCreatePadParam->pParam);
				RegisterModulesInPadHandler();
				break;
			case PS2VM_MSG_DESTROYPAD:
				DELETEPTR(m_pPad);
				break;
			case PS2VM_MSG_SAVESTATE:
				nRetValue = SaveVMState((const char*)Msg.pParam);
				break;
			case PS2VM_MSG_LOADSTATE:
				nRetValue = LoadVMState((const char*)Msg.pParam);
				break;
			case PS2VM_MSG_RESET:
				ResetVM();
				break;
			}

			m_MsgBox.FlushMessage(nRetValue);
		}
		if(nEnd) break;
		if(m_nStatus == PS2VM_STATUS_PAUSED)
		{
			Sleep(100);
		}
		if(m_nStatus == PS2VM_STATUS_RUNNING)
		{
			RET_CODE nRet;

#ifdef PROFILE
			CProfiler::GetInstance().BeginIteration();
			CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif

#if (DEBUGGER_INCLUDED && VU_DEBUG)
			if(CVIF::IsVU1Running())
			{
				nRet = m_VU1.Execute(100000);
				if(nRet == RET_CODE_BREAKPOINT)
				{
					printf("PS2VM: (VectorUnit1) Breakpoint encountered at 0x%0.8X.\r\n", m_VU1.m_State.nPC);
					m_nStatus = PS2VM_STATUS_PAUSED;
					m_OnMachineStateChange.Notify(0);
					m_OnRunningStateChange.Notify(0);
				}
			}
			else
#endif
			{
				if(m_EE.MustBreak())
				{
					nRet = RET_CODE_BREAKPOINT;
				}
				else
				{
					nRet = m_EE.Execute(5000);
					m_nVBlankTicks -= (5000 - m_EE.m_nQuota);
					CTimer::Count(5000 - m_EE.m_nQuota);
					if((int)m_nVBlankTicks <= 0)
					{
						m_nInVBlank = !m_nInVBlank;
						if(m_nInVBlank)
						{
							m_nVBlankTicks = VBLANKTICKS;
							m_pGS->SetVBlank();
							
							//Old Flipping Method
							//m_pGS->Flip();
							//////

							m_pPad->Update();
							m_OnNewFrame.Notify(NULL);
						}
						else
						{
							m_nVBlankTicks = SCREENTICKS;
							m_pGS->ResetVBlank();
						}
					}
				}

#ifdef PROFILE
			CProfiler::GetInstance().EndZone();
			CProfiler::GetInstance().EndIteration();
#endif

				if(nRet == RET_CODE_BREAKPOINT)
				{
					printf("PS2VM: (EmotionEngine) Breakpoint encountered at 0x%0.8X.\r\n", m_EE.m_State.nPC);
					m_nStatus = PS2VM_STATUS_PAUSED;
					m_OnMachineStateChange.Notify(0);
					m_OnRunningStateChange.Notify(0);
					continue;
				}
			}
		}
	}
}
