#include <cstring>
#include <stddef.h>
#include <stdlib.h>
#include <exception>
#include "string_format.h"
#include "PS2OS.h"
#include "StdStream.h"
#ifdef __ANDROID__
#include "android/AssetStream.h"
#include "android/ContentStream.h"
#include "android/ContentUtils.h"
#endif
#include "../Ps2Const.h"
#include "../DiskUtils.h"
#include "../ElfFile.h"
#include "../COP_SCU.h"
#include "../uint128.h"
#include "../Log.h"
#include "../iop/IopBios.h"
#include "DMAC.h"
#include "INTC.h"
#include "Timer.h"
#include "SIF.h"
#include "EEAssembler.h"
#include "PathUtils.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/FilteringNodeIterator.h"
#include "StdStreamUtils.h"
#include "AppConfig.h"
#include "PS2VM_Preferences.h"

#define BIOS_ADDRESS_STATE_BASE 0x1000
#define BIOS_ADDRESS_STATE_ITEM(a) (BIOS_ADDRESS_STATE_BASE + offsetof(BIOS_STATE, a))

#define BIOS_ADDRESS_INTERRUPT_THREAD_CONTEXT (BIOS_ADDRESS_STATE_BASE + sizeof(BIOS_STATE))
#define BIOS_ADDRESS_INTCHANDLER_BASE 0x0000A000
#define BIOS_ADDRESS_DMACHANDLER_BASE 0x0000C000
#define BIOS_ADDRESS_SEMAPHORE_BASE 0x0000E000
#define BIOS_ADDRESS_CUSTOMSYSCALL_BASE 0x00010000
#define BIOS_ADDRESS_ALARM_BASE 0x00010800
#define BIOS_ADDRESS_THREAD_BASE 0x00011000
#define BIOS_ADDRESS_KERNELSTACK_TOP 0x00030000

#define BIOS_ADDRESS_BASE 0x1FC00000
#define BIOS_ADDRESS_INTERRUPTHANDLER 0x1FC00200
#define BIOS_ADDRESS_DMACHANDLER 0x1FC01000
#define BIOS_ADDRESS_INTCHANDLER 0x1FC02000
#define BIOS_ADDRESS_THREADEPILOG 0x1FC03000
#define BIOS_ADDRESS_IDLETHREADPROC 0x1FC03100
#define BIOS_ADDRESS_ALARMHANDLER 0x1FC03200

#define BIOS_ID_BASE 1

//Some notes about SEMA_ID_BASE:
// 2K games (ex.: NBA 2K12) seem to have a bug where they wait for a semaphore they don't own.
// The semaphore id is never set and the game will use the id in WaitSema and SignalSema.
// On a real PS2, this seem to work because ids start at 0. The first semaphore created in the
// game's lifetime doesn't seem to be used, so this bug probably doesn't have any side effect.
#define SEMA_ID_BASE 0

#define PATCHESFILENAME "patches.xml"
#define LOG_NAME ("ps2os")

#define SYSCALL_CUSTOM_RESCHEDULE 0x666
#define SYSCALL_CUSTOM_EXITINTERRUPT 0x667

#define SYSCALL_NAME_EXIT "osExit"
#define SYSCALL_NAME_LOADEXECPS2 "osLoadExecPS2"
#define SYSCALL_NAME_EXECPS2 "osExecPS2"
#define SYSCALL_NAME_SETVTLBREFILLHANDLER "osSetVTLBRefillHandler"
#define SYSCALL_NAME_SETVCOMMONHANDLER "osSetVCommonHandler"
#define SYSCALL_NAME_ADDINTCHANDLER "osAddIntcHandler"
#define SYSCALL_NAME_REMOVEINTCHANDLER "osRemoveIntcHandler"
#define SYSCALL_NAME_ADDDMACHANDLER "osAddDmacHandler"
#define SYSCALL_NAME_REMOVEDMACHANDLER "osRemoveDmacHandler"
#define SYSCALL_NAME_ENABLEINTC "osEnableIntc"
#define SYSCALL_NAME_DISABLEINTC "osDisableIntc"
#define SYSCALL_NAME_ENABLEDMAC "osEnableDmac"
#define SYSCALL_NAME_DISABLEDMAC "osDisableDmac"
#define SYSCALL_NAME_SETALARM "osSetAlarm"
#define SYSCALL_NAME_IENABLEINTC "osiEnableIntc"
#define SYSCALL_NAME_IDISABLEINTC "osiDisableIntc"
#define SYSCALL_NAME_IENABLEDMAC "osiEnableDmac"
#define SYSCALL_NAME_IDISABLEDMAC "osiDisableDmac"
#define SYSCALL_NAME_ISETALARM "osiSetAlarm"
#define SYSCALL_NAME_IRELEASEALARM "osiReleaseAlarm"
#define SYSCALL_NAME_CREATETHREAD "osCreateThread"
#define SYSCALL_NAME_DELETETHREAD "osDeleteThread"
#define SYSCALL_NAME_STARTTHREAD "osStartThread"
#define SYSCALL_NAME_EXITTHREAD "osExitThread"
#define SYSCALL_NAME_EXITDELETETHREAD "osExitDeleteThread"
#define SYSCALL_NAME_TERMINATETHREAD "osTerminateThread"
#define SYSCALL_NAME_CHANGETHREADPRIORITY "osChangeThreadPriority"
#define SYSCALL_NAME_ICHANGETHREADPRIORITY "osiChangeThreadPriority"
#define SYSCALL_NAME_ROTATETHREADREADYQUEUE "osRotateThreadReadyQueue"
#define SYSCALL_NAME_RELEASEWAITTHREAD "osReleaseWaitThread"
#define SYSCALL_NAME_IRELEASEWAITTHREAD "osiReleaseWaitThread"
#define SYSCALL_NAME_GETTHREADID "osGetThreadId"
#define SYSCALL_NAME_REFERTHREADSTATUS "osReferThreadStatus"
#define SYSCALL_NAME_IREFERTHREADSTATUS "osiReferThreadStatus"
#define SYSCALL_NAME_GETOSDCONFIGPARAM "osGetOsdConfigParam"
#define SYSCALL_NAME_GETCOP0 "osGetCop0"
#define SYSCALL_NAME_SLEEPTHREAD "osSleepThread"
#define SYSCALL_NAME_WAKEUPTHREAD "osWakeupThread"
#define SYSCALL_NAME_IWAKEUPTHREAD "osiWakeupThread"
#define SYSCALL_NAME_CANCELWAKEUPTHREAD "osCancelWakeupThread"
#define SYSCALL_NAME_ICANCELWAKEUPTHREAD "osiCancelWakeupThread"
#define SYSCALL_NAME_SUSPENDTHREAD "osSuspendThread"
#define SYSCALL_NAME_ISUSPENDTHREAD "osiSuspendThread"
#define SYSCALL_NAME_RESUMETHREAD "osResumeThread"
#define SYSCALL_NAME_IRESUMETHREAD "osiResumeThread"
#define SYSCALL_NAME_ENDOFHEAP "osEndOfHeap"
#define SYSCALL_NAME_CREATESEMA "osCreateSema"
#define SYSCALL_NAME_DELETESEMA "osDeleteSema"
#define SYSCALL_NAME_SIGNALSEMA "osSignalSema"
#define SYSCALL_NAME_ISIGNALSEMA "osiSignalSema"
#define SYSCALL_NAME_WAITSEMA "osWaitSema"
#define SYSCALL_NAME_POLLSEMA "osPollSema"
#define SYSCALL_NAME_IPOLLSEMA "osiPollSema"
#define SYSCALL_NAME_REFERSEMASTATUS "osReferSemaStatus"
#define SYSCALL_NAME_IREFERSEMASTATUS "osiReferSemaStatus"
#define SYSCALL_NAME_FLUSHCACHE "osFlushCache"
#define SYSCALL_NAME_SIFSTOPDMA "osSifStopDma"
#define SYSCALL_NAME_GSGETIMR "osGsGetIMR"
#define SYSCALL_NAME_GSPUTIMR "osGsPutIMR"
#define SYSCALL_NAME_SETVSYNCFLAG "osSetVSyncFlag"
#define SYSCALL_NAME_SETSYSCALL "osSetSyscall"
#define SYSCALL_NAME_SIFDMASTAT "osSifDmaStat"
#define SYSCALL_NAME_SIFSETDMA "osSifSetDma"
#define SYSCALL_NAME_SIFSETDCHAIN "osSifSetDChain"
#define SYSCALL_NAME_DECI2CALL "osDeci2Call"
#define SYSCALL_NAME_MACHINETYPE "osMachineType"

#ifdef DEBUGGER_INCLUDED

// clang-format off
const CPS2OS::SYSCALL_NAME CPS2OS::g_syscallNames[] =
{
	{0x0004, SYSCALL_NAME_EXIT},
	{0x0006, SYSCALL_NAME_LOADEXECPS2},
	{0x0007, SYSCALL_NAME_EXECPS2},
	{0x000D, SYSCALL_NAME_SETVTLBREFILLHANDLER},
	{0x000E, SYSCALL_NAME_SETVCOMMONHANDLER},
	{0x0010, SYSCALL_NAME_ADDINTCHANDLER},
	{0x0011, SYSCALL_NAME_REMOVEINTCHANDLER},
	{0x0012, SYSCALL_NAME_ADDDMACHANDLER},
	{0x0013, SYSCALL_NAME_REMOVEDMACHANDLER},
	{0x0014, SYSCALL_NAME_ENABLEINTC},
	{0x0015, SYSCALL_NAME_DISABLEINTC},
	{0x0016, SYSCALL_NAME_ENABLEDMAC},
	{0x0017, SYSCALL_NAME_DISABLEDMAC},
	{0x0018, SYSCALL_NAME_SETALARM},
	{0x001A, SYSCALL_NAME_IENABLEINTC},
	{0x001B, SYSCALL_NAME_IDISABLEINTC},
	{0x001C, SYSCALL_NAME_IENABLEDMAC},
	{0x001D, SYSCALL_NAME_IDISABLEDMAC},
	{0x001E, SYSCALL_NAME_ISETALARM},
	{0x001F, SYSCALL_NAME_IRELEASEALARM},
	{0x0020, SYSCALL_NAME_CREATETHREAD},
	{0x0021, SYSCALL_NAME_DELETETHREAD},
	{0x0022, SYSCALL_NAME_STARTTHREAD},
	{0x0023, SYSCALL_NAME_EXITTHREAD},
	{0x0024, SYSCALL_NAME_EXITDELETETHREAD},
	{0x0025, SYSCALL_NAME_TERMINATETHREAD},
	{0x0029, SYSCALL_NAME_CHANGETHREADPRIORITY},
	{0x002A, SYSCALL_NAME_ICHANGETHREADPRIORITY},
	{0x002B, SYSCALL_NAME_ROTATETHREADREADYQUEUE},
	{0x002D, SYSCALL_NAME_RELEASEWAITTHREAD},
	{0x002E, SYSCALL_NAME_IRELEASEWAITTHREAD},
	{0x002F, SYSCALL_NAME_GETTHREADID},
	{0x0030, SYSCALL_NAME_REFERTHREADSTATUS},
	{0x0031, SYSCALL_NAME_IREFERTHREADSTATUS},
	{0x0032, SYSCALL_NAME_SLEEPTHREAD},
	{0x0033, SYSCALL_NAME_WAKEUPTHREAD},
	{0x0034, SYSCALL_NAME_IWAKEUPTHREAD},
	{0x0035, SYSCALL_NAME_CANCELWAKEUPTHREAD},
	{0x0036, SYSCALL_NAME_ICANCELWAKEUPTHREAD},
	{0x0037, SYSCALL_NAME_SUSPENDTHREAD},
	{0x0038, SYSCALL_NAME_ISUSPENDTHREAD},
	{0x0039, SYSCALL_NAME_RESUMETHREAD},
	{0x003A, SYSCALL_NAME_IRESUMETHREAD},
	{0x003E, SYSCALL_NAME_ENDOFHEAP},
	{0x0040, SYSCALL_NAME_CREATESEMA},
	{0x0041, SYSCALL_NAME_DELETESEMA},
	{0x0042, SYSCALL_NAME_SIGNALSEMA},
	{0x0043, SYSCALL_NAME_ISIGNALSEMA},
	{0x0044, SYSCALL_NAME_WAITSEMA},
	{0x0045, SYSCALL_NAME_POLLSEMA},
	{0x0046, SYSCALL_NAME_IPOLLSEMA},
	{0x0047, SYSCALL_NAME_REFERSEMASTATUS},
	{0x0048, SYSCALL_NAME_IREFERSEMASTATUS},
	{0x004B, SYSCALL_NAME_GETOSDCONFIGPARAM},
	{0x0063, SYSCALL_NAME_GETCOP0},
	{0x0064, SYSCALL_NAME_FLUSHCACHE},
	{0x006B, SYSCALL_NAME_SIFSTOPDMA},
	{0x0070, SYSCALL_NAME_GSGETIMR},
	{0x0071, SYSCALL_NAME_GSPUTIMR},
	{0x0073, SYSCALL_NAME_SETVSYNCFLAG},
	{0x0074, SYSCALL_NAME_SETSYSCALL},
	{0x0076, SYSCALL_NAME_SIFDMASTAT},
	{0x0077, SYSCALL_NAME_SIFSETDMA},
	{0x0078, SYSCALL_NAME_SIFSETDCHAIN},
	{0x007C, SYSCALL_NAME_DECI2CALL},
	{0x007E, SYSCALL_NAME_MACHINETYPE},
	{0x0000, nullptr}
};
// clang-format on

#endif

CPS2OS::CPS2OS(CMIPS& ee, uint8* ram, uint8* bios, uint8* spr, CGSHandler*& gs, CSIF& sif, CIopBios& iopBios)
    : m_ee(ee)
    , m_gs(gs)
    , m_ram(ram)
    , m_bios(bios)
    , m_spr(spr)
    , m_sif(sif)
    , m_libMc2(ram, *this, iopBios)
    , m_iopBios(iopBios)
    , m_threads(reinterpret_cast<THREAD*>(m_ram + BIOS_ADDRESS_THREAD_BASE), BIOS_ID_BASE, MAX_THREAD)
    , m_semaphores(reinterpret_cast<SEMAPHORE*>(m_ram + BIOS_ADDRESS_SEMAPHORE_BASE), SEMA_ID_BASE, MAX_SEMAPHORE)
    , m_intcHandlers(reinterpret_cast<INTCHANDLER*>(m_ram + BIOS_ADDRESS_INTCHANDLER_BASE), BIOS_ID_BASE, MAX_INTCHANDLER)
    , m_dmacHandlers(reinterpret_cast<DMACHANDLER*>(m_ram + BIOS_ADDRESS_DMACHANDLER_BASE), BIOS_ID_BASE, MAX_DMACHANDLER)
    , m_alarms(reinterpret_cast<ALARM*>(m_ram + BIOS_ADDRESS_ALARM_BASE), BIOS_ID_BASE, MAX_ALARM)
    , m_state(reinterpret_cast<BIOS_STATE*>(m_ram + BIOS_ADDRESS_STATE_BASE))
    , m_currentThreadId(&m_state->currentThreadId)
    , m_idleThreadId(&m_state->idleThreadId)
    , m_tlblExceptionHandler(&m_state->tlblExceptionHandler)
    , m_tlbsExceptionHandler(&m_state->tlbsExceptionHandler)
    , m_trapExceptionHandler(&m_state->trapExceptionHandler)
    , m_sifDmaNextIdx(&m_state->sifDmaNextIdx)
    , m_sifDmaTimes(m_state->sifDmaTimes)
    , m_threadSchedule(m_threads, &m_state->threadScheduleBase)
    , m_intcHandlerQueue(m_intcHandlers, &m_state->intcHandlerQueueBase)
    , m_dmacHandlerQueue(m_dmacHandlers, &m_state->dmacHandlerQueueBase)
{
	static_assert((BIOS_ADDRESS_SEMAPHORE_BASE + (sizeof(SEMAPHORE) * MAX_SEMAPHORE)) <= BIOS_ADDRESS_CUSTOMSYSCALL_BASE, "Semaphore overflow");

	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_SYSTEM_LANGUAGE, static_cast<uint32>(OSD_LANGUAGE::JAPANESE));
}

CPS2OS::~CPS2OS()
{
	Release();
}

void CPS2OS::Initialize(uint32 ramSize)
{
	m_elf = nullptr;
	m_idleEvaluator.Reset();
	m_ramSize = ramSize;

	//Specially selected next thread id to make sure main thread id is 1.
	//First thread to be created is the idle thread, which will cause next id to become 1
	//for when main thread will be created through SetupThread.
	m_state->allocateThreadNextId = m_threads.GetIdBase() + MAX_THREAD - 1;

	SetVsyncFlagPtrs(0, 0);
	UpdateTLBEnabledState();

	AssembleCustomSyscallHandler();
	AssembleInterruptHandler();
	AssembleDmacHandler();
	AssembleIntcHandler();
	AssembleThreadEpilog();
	AssembleIdleThreadProc();
	AssembleAlarmHandler();
	CreateIdleThread();

	m_ee.m_State.nPC = BIOS_ADDRESS_IDLETHREADPROC;
	m_ee.m_State.nCOP0[CCOP_SCU::STATUS] |= (CMIPS::STATUS_IE | CMIPS::STATUS_EIE);

	//The BIOS enables TIMER2 and TIMER3 for alarm purposes and others, some games (ex.: SoulCalibur 3, Ape Escape 2) assume timer is counting.
	m_ee.m_pMemoryMap->SetWord(CTimer::T2_MODE, CTimer::MODE_COUNT_ENABLE | CTimer::MODE_EQUAL_FLAG | CTimer::MODE_OVERFLOW_FLAG);
	m_ee.m_pMemoryMap->SetWord(CTimer::T3_MODE, CTimer::MODE_CLOCK_SELECT_EXTERNAL | CTimer::MODE_COUNT_ENABLE | CTimer::MODE_EQUAL_FLAG | CTimer::MODE_OVERFLOW_FLAG);
}

void CPS2OS::Release()
{
	UnloadExecutable();
}

bool CPS2OS::IsIdle() const
{
	return m_ee.CanGenerateInterrupt() &&
	       ((m_currentThreadId == m_idleThreadId) || m_idleEvaluator.IsIdle());
}

void CPS2OS::BootFromFile(const fs::path& execPath)
{
	auto stream = [&]() -> std::unique_ptr<Framework::CStream> {
#ifdef __ANDROID__
		if(Framework::Android::CContentUtils::IsContentPath(execPath))
		{
			auto uri = Framework::Android::CContentUtils::BuildUriFromPath(execPath);
			return std::make_unique<Framework::Android::CContentStream>(uri.c_str(), "r");
		}
#endif
		return std::make_unique<Framework::CStdStream>(execPath.native().c_str(), Framework::GetInputStdStreamMode<fs::path::string_type>());
	}();
	auto virtualExecutablePath = "host:" + execPath.filename().string();
	LoadELF(stream.get(), virtualExecutablePath.c_str(), ArgumentList());
}

void CPS2OS::BootFromVirtualPath(const char* executablePath, const ArgumentList& arguments)
{
	auto ioman = m_iopBios.GetIoman();

	uint32 handle = ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, executablePath);
	if(static_cast<int32>(handle) < 0)
	{
		throw std::runtime_error("Couldn't open executable specified by virtual path.");
	}

	try
	{
		Framework::CStream* file(ioman->GetFileStream(handle));
		LoadELF(file, executablePath, arguments);
	}
	catch(...)
	{
		throw std::runtime_error("Error occured while reading ELF executable from virtual path.");
	}
	ioman->Close(handle);
}

void CPS2OS::BootFromCDROM()
{
	std::string executablePath;
	auto ioman = m_iopBios.GetIoman();

	{
		uint32 handle = ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, "cdrom0:SYSTEM.CNF");
		if(static_cast<int32>(handle) < 0)
		{
			throw std::runtime_error("No 'SYSTEM.CNF' file found on the cdrom0 device.");
		}

		{
			Framework::CStream* file(ioman->GetFileStream(handle));
			auto systemConfig = DiskUtils::ParseSystemConfigFile(file);
			auto bootItemIterator = systemConfig.find("BOOT2");
			if(bootItemIterator != std::end(systemConfig))
			{
				executablePath = bootItemIterator->second;
			}
		}

		ioman->Close(handle);
	}

	if(executablePath.length() == 0)
	{
		throw std::runtime_error("Error parsing 'SYSTEM.CNF' for a BOOT2 value.");
	}

	BootFromVirtualPath(executablePath.c_str(), ArgumentList());
}

CELF32* CPS2OS::GetELF()
{
	return m_elf.get();
}

const char* CPS2OS::GetExecutableName() const
{
	return m_executableName.c_str();
}

std::pair<uint32, uint32> CPS2OS::GetExecutableRange() const
{
	uint32 minAddr = 0xFFFFFFF0;
	uint32 maxAddr = 0x00000000;
	const auto& header = m_elf->GetHeader();

	for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
	{
		auto p = m_elf->GetProgram(i);
		if(p != NULL)
		{
			//Wild Arms: Alter Code F has zero sized program headers
			if(p->nFileSize == 0) continue;
			if(!(p->nFlags & ELF::PF_X)) continue;
			uint32 end = p->nVAddress + p->nFileSize;
			if(end >= m_ramSize) continue;
			minAddr = std::min<uint32>(minAddr, p->nVAddress);
			maxAddr = std::max<uint32>(maxAddr, end);
		}
	}

	return std::pair<uint32, uint32>(minAddr, maxAddr);
}

void CPS2OS::LoadELF(Framework::CStream* stream, const char* executablePath, const ArgumentList& arguments)
{
	auto elf = std::make_unique<CElf32File>(*stream);
	const auto& header = elf->GetHeader();

	//Check for MIPS CPU
	if(header.nCPU != ELF::EM_MIPS)
	{
		throw std::runtime_error("Invalid target CPU. Must be MIPS.");
	}

	if(header.nType != ELF::ET_EXEC)
	{
		throw std::runtime_error("Not an executable ELF file.");
	}

	UnloadExecutable();

	m_elf = std::move(elf);

	m_currentArguments.clear();
	m_currentArguments.push_back(executablePath);
	m_currentArguments.insert(m_currentArguments.end(), arguments.begin(), arguments.end());

	m_executableName =
	    [&]() {
		    auto executableName = reinterpret_cast<const char*>(strchr(executablePath, ':'));
		    if(!executableName) return executablePath;
		    executableName++;
		    if(executableName[0] == '/' || executableName[0] == '\\') executableName++;
		    return executableName;
	    }();

	LoadExecutableInternal();
	ApplyPatches();

	OnExecutableChange();

	CLog::GetInstance().Print(LOG_NAME, "Loaded '%s' executable file.\r\n", executablePath);
}

void CPS2OS::LoadExecutableInternal()
{
	//Copy program in main RAM
	const auto& header = m_elf->GetHeader();
	for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
	{
		auto p = m_elf->GetProgram(i);
		if(p != nullptr)
		{
			if(p->nFileSize == 0) continue;
			if(p->nVAddress >= m_ramSize)
			{
				assert(false);
				continue;
			}
			memcpy(m_ram + p->nVAddress, m_elf->GetContent() + p->nOffset, p->nFileSize);
		}
	}

	m_ee.m_State.nPC = header.nEntryPoint;
	m_ee.m_State.nGPR[CMIPS::A0].nV[0] = header.nEntryPoint;

#ifdef DEBUGGER_INCLUDED
	std::pair<uint32, uint32> executableRange = GetExecutableRange();
	uint32 minAddr = executableRange.first;
	uint32 maxAddr = executableRange.second & ~0x03;

	m_ee.m_analysis->Clear();
	m_ee.m_analysis->Analyse(minAddr, maxAddr, header.nEntryPoint);

	//Tag system calls
	for(uint32 address = minAddr; address < maxAddr; address += 4)
	{
		//Check for SYSCALL opcode
		uint32 opcode = *reinterpret_cast<uint32*>(m_ram + address);
		if(opcode == 0x0000000C)
		{
			//Check the opcode before and after it
			uint32 addiu = *reinterpret_cast<uint32*>(m_ram + address - 4);
			uint32 jr = *reinterpret_cast<uint32*>(m_ram + address + 4);
			if(
			    (jr == 0x03E00008) &&
			    (addiu & 0xFFFF0000) == 0x24030000)
			{
				//We have it!
				int16 syscallId = static_cast<int16>(addiu);
				if(syscallId & 0x8000)
				{
					syscallId = 0 - syscallId;
				}
				char syscallName[256];
				int syscallNameIndex = -1;
				for(int i = 0; g_syscallNames[i].name != NULL; i++)
				{
					if(g_syscallNames[i].id == syscallId)
					{
						syscallNameIndex = i;
						break;
					}
				}
				if(syscallNameIndex != -1)
				{
					strncpy(syscallName, g_syscallNames[syscallNameIndex].name, 256);
				}
				else
				{
					sprintf(syscallName, "syscall_%04X", syscallId);
				}
				m_ee.m_Functions.InsertTag(address - 4, syscallName);
			}
		}
	}

#endif
}

void CPS2OS::UnloadExecutable()
{
	if(!m_elf) return;

	OnExecutableUnloading();

	m_elf.reset();
}

uint32 CPS2OS::LoadExecutable(const char* path, const char* section)
{
	auto ioman = m_iopBios.GetIoman();

	uint32 handle = ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, path);
	if(static_cast<int32>(handle) < 0)
	{
		return -1;
	}

	uint32 result = 0;

	//We don't support loading anything else than all sections
	assert(strcmp(section, "all") == 0);

	auto fileStream(ioman->GetFileStream(handle));

	//Load all program sections
	{
		CElf32File executable(*fileStream);
		const auto& header = executable.GetHeader();
		for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
		{
			auto p = executable.GetProgram(i);
			if(p)
			{
				memcpy(m_ram + p->nVAddress, executable.GetContent() + p->nOffset, p->nFileSize);
			}
		}

		result = executable.GetHeader().nEntryPoint;
	}

	//Flush all instruction cache
	OnRequestInstructionCacheFlush();

	ioman->Close(handle);

	return result;
}

void CPS2OS::ApplyPatches()
{
	std::unique_ptr<Framework::Xml::CNode> document;
	try
	{
#ifdef __ANDROID__
		Framework::Android::CAssetStream patchesStream(PATCHESFILENAME);
#else
		auto patchesPath = Framework::PathUtils::GetAppResourcesPath() / PATCHESFILENAME;
		Framework::CStdStream patchesStream(Framework::CreateInputStdStream(patchesPath.native()));
#endif
		document = Framework::Xml::CParser::ParseDocument(patchesStream);
		if(!document) return;
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Print(LOG_NAME, "Failed to open patch definition file: %s.\r\n", exception.what());
		return;
	}

	auto patchesNode = document->Select("Patches");
	if(patchesNode == NULL)
	{
		return;
	}

	for(Framework::Xml::CFilteringNodeIterator itNode(patchesNode, "Executable"); !itNode.IsEnd(); itNode++)
	{
		auto executableNode = (*itNode);

		const char* name = executableNode->GetAttribute("Name");
		if(name == NULL) continue;

		if(!strcmp(name, GetExecutableName()))
		{
			//Found the right executable
			unsigned int patchCount = 0;

			for(Framework::Xml::CFilteringNodeIterator itNode(executableNode, "Patch"); !itNode.IsEnd(); itNode++)
			{
				auto patch = (*itNode);

				const char* addressString = patch->GetAttribute("Address");
				const char* valueString = patch->GetAttribute("Value");

				if(addressString == nullptr) continue;
				if(valueString == nullptr) continue;

				uint32 value = 0, address = 0;
				if(sscanf(addressString, "%x", &address) == 0) continue;
				if(sscanf(valueString, "%x", &value) == 0) continue;

				*(uint32*)&m_ram[address] = value;

				patchCount++;
			}

			CLog::GetInstance().Print(LOG_NAME, "Applied %i patch(es).\r\n", patchCount);

			break;
		}
	}
}

void CPS2OS::AssembleCustomSyscallHandler()
{
	CMIPSAssembler assembler((uint32*)&m_bios[0x100]);

	//Epilogue
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFF0);
	assembler.SD(CMIPS::RA, 0x0000, CMIPS::SP);

	//Load the function address off the table at 0x80010000
	assembler.SLL(CMIPS::T0, CMIPS::V1, 2);
	assembler.LUI(CMIPS::T1, 0x8001);
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);
	assembler.LW(CMIPS::T0, 0x0000, CMIPS::T0);

	//And the address with 0x1FFFFFFF
	assembler.LUI(CMIPS::T1, 0x1FFF);
	assembler.ORI(CMIPS::T1, CMIPS::T1, 0xFFFF);
	assembler.AND(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Jump to the system call address
	assembler.JALR(CMIPS::T0);
	assembler.NOP();

	//Prologue
	assembler.LD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0x0010);
	assembler.ERET();
}

void CPS2OS::AssembleInterruptHandler()
{
	CEEAssembler assembler(reinterpret_cast<uint32*>(&m_bios[BIOS_ADDRESS_INTERRUPTHANDLER - BIOS_ADDRESS_BASE]));

	auto processDmacLabel = assembler.CreateLabel();
	auto doneLabel = assembler.CreateLabel();

	const uint32 stackFrameSize = 0x230;

	//Invariant - Current thread is idle thread
	//This means we don't need to save/load its context because
	//it doesn't have any

	//Epilogue (allocate stackFrameSize bytes)
	assembler.LI(CMIPS::K0, BIOS_ADDRESS_KERNELSTACK_TOP);
	assembler.ADDIU(CMIPS::K0, CMIPS::K0, 0x10000 - stackFrameSize);

	//Save EPC
	assembler.MFC0(CMIPS::T0, CCOP_SCU::EPC);
	assembler.SW(CMIPS::T0, 0x0220, CMIPS::K0);

	//Set SP
	assembler.ADDU(CMIPS::SP, CMIPS::K0, CMIPS::R0);

	//Disable interrupts. Used by some games (ie.: RE4) to check interrupt context.
	assembler.MFC0(CMIPS::T0, CCOP_SCU::STATUS);
	assembler.LI(CMIPS::T1, ~CMIPS::STATUS_IE);
	assembler.AND(CMIPS::T0, CMIPS::T0, CMIPS::T1);
	assembler.MTC0(CMIPS::T0, CCOP_SCU::STATUS);

	//Get CAUSE register
	assembler.MFC0(CMIPS::T0, CCOP_SCU::CAUSE);
	assembler.ADDIU(CMIPS::T1, CMIPS::R0, CCOP_SCU::CAUSE_IP_3);
	assembler.AND(CMIPS::T0, CMIPS::T0, CMIPS::T1);
	assembler.BNE(CMIPS::T0, CMIPS::R0, processDmacLabel);
	assembler.NOP();

	//Process INTC interrupt
	{
		//Get INTC status
		assembler.LI(CMIPS::T0, CINTC::INTC_STAT);
		assembler.LW(CMIPS::S0, 0x0000, CMIPS::T0);

		//Get INTC mask
		assembler.LI(CMIPS::T1, CINTC::INTC_MASK);
		assembler.LW(CMIPS::S1, 0x0000, CMIPS::T1);

		//Get cause
		assembler.AND(CMIPS::S0, CMIPS::S0, CMIPS::S1);

		static const auto generateIntHandler =
		    [](CMIPSAssembler& assembler, uint32 line) {
			    auto skipIntHandlerLabel = assembler.CreateLabel();

			    //Check cause
			    assembler.ANDI(CMIPS::T0, CMIPS::S0, (1 << line));
			    assembler.BEQ(CMIPS::R0, CMIPS::T0, skipIntHandlerLabel);
			    assembler.NOP();

			    //Process handlers
			    assembler.ADDIU(CMIPS::A0, CMIPS::R0, line);
			    assembler.JAL(BIOS_ADDRESS_INTCHANDLER);
			    assembler.NOP();

			    if(line == CINTC::INTC_LINE_TIMER3)
			    {
				    assembler.JAL(BIOS_ADDRESS_ALARMHANDLER);
				    assembler.NOP();
			    }

			    assembler.MarkLabel(skipIntHandlerLabel);
		    };

		generateIntHandler(assembler, CINTC::INTC_LINE_GS);
		generateIntHandler(assembler, CINTC::INTC_LINE_VBLANK_START);
		generateIntHandler(assembler, CINTC::INTC_LINE_VBLANK_END);
		generateIntHandler(assembler, CINTC::INTC_LINE_VIF0);
		generateIntHandler(assembler, CINTC::INTC_LINE_VIF1);
		generateIntHandler(assembler, CINTC::INTC_LINE_IPU);
		generateIntHandler(assembler, CINTC::INTC_LINE_TIMER0);
		generateIntHandler(assembler, CINTC::INTC_LINE_TIMER1);
		generateIntHandler(assembler, CINTC::INTC_LINE_TIMER2);
		generateIntHandler(assembler, CINTC::INTC_LINE_TIMER3);

		assembler.BEQ(CMIPS::R0, CMIPS::R0, doneLabel);
		assembler.NOP();
	}

	assembler.MarkLabel(processDmacLabel);

	//Process DMAC interrupt
	{
		assembler.JAL(BIOS_ADDRESS_DMACHANDLER);
		assembler.NOP();
	}

	assembler.MarkLabel(doneLabel);

	//Make sure interrupts are enabled (This is needed by some games that play
	//with the status register in interrupt handlers and is done by the EE BIOS)
	assembler.MFC0(CMIPS::T0, CCOP_SCU::STATUS);
	assembler.ORI(CMIPS::T0, CMIPS::T0, CMIPS::STATUS_IE);
	assembler.MTC0(CMIPS::T0, CCOP_SCU::STATUS);

	//Prologue

	//Move back SP into K0 before restoring state
	assembler.ADDIU(CMIPS::K0, CMIPS::SP, CMIPS::R0);
	//Read EPC from 0x220
	assembler.LW(CMIPS::A0, 0x0220, CMIPS::K0);

	assembler.ADDIU(CMIPS::V1, CMIPS::R0, SYSCALL_CUSTOM_EXITINTERRUPT);
	assembler.SYSCALL();
}

void CPS2OS::AssembleDmacHandler()
{
	CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_bios[BIOS_ADDRESS_DMACHANDLER - BIOS_ADDRESS_BASE]));

	auto checkHandlerLabel = assembler.CreateLabel();
	auto testChannelLabel = assembler.CreateLabel();
	auto skipChannelLabel = assembler.CreateLabel();

	auto channelCounterRegister = CMIPS::S0;
	auto interruptStatusRegister = CMIPS::S1;
	auto nextIdPtrRegister = CMIPS::S2;

	auto nextIdRegister = CMIPS::T2;

	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE0);
	assembler.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.SD(CMIPS::S1, 0x0010, CMIPS::SP);
	assembler.SD(CMIPS::S2, 0x0018, CMIPS::SP);

	//Load the DMA interrupt status
	assembler.LI(CMIPS::T0, CDMAC::D_STAT);
	assembler.LW(CMIPS::T0, 0x0000, CMIPS::T0);

	assembler.SRL(CMIPS::T1, CMIPS::T0, 16);
	assembler.AND(interruptStatusRegister, CMIPS::T0, CMIPS::T1);

	//Initialize channel counter
	assembler.ADDIU(channelCounterRegister, CMIPS::R0, 0x000E);

	assembler.MarkLabel(testChannelLabel);

	//Check if that specific DMA channel interrupt is the cause
	assembler.ORI(CMIPS::T0, CMIPS::R0, 0x0001);
	assembler.SLLV(CMIPS::T0, CMIPS::T0, CMIPS::S0);
	assembler.AND(CMIPS::T0, CMIPS::T0, CMIPS::S1);
	assembler.BEQ(CMIPS::T0, CMIPS::R0, skipChannelLabel);
	assembler.NOP();

	//Clear interrupt
	assembler.LI(CMIPS::T1, CDMAC::D_STAT);
	assembler.SW(CMIPS::T0, 0x0000, CMIPS::T1);

	//Initialize DMAC handler loop
	assembler.LI(nextIdPtrRegister, BIOS_ADDRESS_STATE_ITEM(dmacHandlerQueueBase));

	assembler.MarkLabel(checkHandlerLabel);

	//Check
	assembler.LW(nextIdRegister, 0, nextIdPtrRegister);
	assembler.BEQ(nextIdRegister, CMIPS::R0, skipChannelLabel);
	assembler.ADDIU(nextIdRegister, nextIdRegister, static_cast<uint16>(-1));

	//Get the address to the current DMACHANDLER structure
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(DMACHANDLER));
	assembler.MULTU(CMIPS::T0, nextIdRegister, CMIPS::T0);
	assembler.LI(CMIPS::T1, BIOS_ADDRESS_DMACHANDLER_BASE);
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Adjust nextIdPtr
	assembler.ADDIU(nextIdPtrRegister, CMIPS::T0, offsetof(INTCHANDLER, nextId));

	//Check if the channel is good one
	assembler.LW(CMIPS::T1, offsetof(DMACHANDLER, channel), CMIPS::T0);
	assembler.BNE(channelCounterRegister, CMIPS::T1, checkHandlerLabel);
	assembler.NOP();

	//Load the necessary stuff
	assembler.LW(CMIPS::T1, offsetof(DMACHANDLER, address), CMIPS::T0);
	assembler.ADDU(CMIPS::A0, channelCounterRegister, CMIPS::R0);
	assembler.LW(CMIPS::A1, offsetof(DMACHANDLER, arg), CMIPS::T0);
	assembler.LW(CMIPS::GP, offsetof(DMACHANDLER, gp), CMIPS::T0);

	//Jump
	assembler.JALR(CMIPS::T1);
	assembler.NOP();

	assembler.BGEZ(CMIPS::V0, checkHandlerLabel);
	assembler.NOP();

	assembler.MarkLabel(skipChannelLabel);

	//Decrement channel counter and test
	assembler.ADDIU(channelCounterRegister, channelCounterRegister, 0xFFFF);
	assembler.BGEZ(channelCounterRegister, testChannelLabel);
	assembler.NOP();

	//Epilogue
	assembler.LD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.LD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.LD(CMIPS::S1, 0x0010, CMIPS::SP);
	assembler.LD(CMIPS::S2, 0x0018, CMIPS::SP);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0x20);
	assembler.JR(CMIPS::RA);
	assembler.NOP();
}

void CPS2OS::AssembleIntcHandler()
{
	CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_bios[BIOS_ADDRESS_INTCHANDLER - BIOS_ADDRESS_BASE]));

	auto checkHandlerLabel = assembler.CreateLabel();
	auto finishLoop = assembler.CreateLabel();

	auto nextIdPtrRegister = CMIPS::S0;
	auto causeRegister = CMIPS::S1;

	auto nextIdRegister = CMIPS::T2;

	//Prologue
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE0);
	assembler.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.SD(CMIPS::S1, 0x0010, CMIPS::SP);

	//Clear INTC cause
	assembler.LI(CMIPS::T1, CINTC::INTC_STAT);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, 0x0001);
	assembler.SLLV(CMIPS::T0, CMIPS::T0, CMIPS::A0);
	assembler.SW(CMIPS::T0, 0x0000, CMIPS::T1);

	//Initialize INTC handler loop
	assembler.LI(nextIdPtrRegister, BIOS_ADDRESS_STATE_ITEM(intcHandlerQueueBase));
	assembler.ADDU(causeRegister, CMIPS::A0, CMIPS::R0);

	assembler.MarkLabel(checkHandlerLabel);

	//Check
	assembler.LW(nextIdRegister, 0, nextIdPtrRegister);
	assembler.BEQ(nextIdRegister, CMIPS::R0, finishLoop);
	assembler.ADDIU(nextIdRegister, nextIdRegister, static_cast<uint16>(-1));

	//Get the address to the current INTCHANDLER structure
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(INTCHANDLER));
	assembler.MULTU(CMIPS::T0, nextIdRegister, CMIPS::T0);
	assembler.LI(CMIPS::T1, BIOS_ADDRESS_INTCHANDLER_BASE);
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Adjust nextIdPtr
	assembler.ADDIU(nextIdPtrRegister, CMIPS::T0, offsetof(INTCHANDLER, nextId));

	//Check if the cause is good one
	assembler.LW(CMIPS::T1, offsetof(INTCHANDLER, cause), CMIPS::T0);
	assembler.BNE(causeRegister, CMIPS::T1, checkHandlerLabel);
	assembler.NOP();

	//Load the necessary stuff
	assembler.LW(CMIPS::T1, offsetof(INTCHANDLER, address), CMIPS::T0);
	assembler.ADDU(CMIPS::A0, causeRegister, CMIPS::R0);
	assembler.LW(CMIPS::A1, offsetof(INTCHANDLER, arg), CMIPS::T0);
	assembler.LW(CMIPS::GP, offsetof(INTCHANDLER, gp), CMIPS::T0);

	//Jump
	assembler.JALR(CMIPS::T1);
	assembler.NOP();

	assembler.BGEZ(CMIPS::V0, checkHandlerLabel);
	assembler.NOP();

	assembler.MarkLabel(finishLoop);

	//Epilogue
	assembler.LD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.LD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.LD(CMIPS::S1, 0x0010, CMIPS::SP);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0x20);
	assembler.JR(CMIPS::RA);
	assembler.NOP();
}

void CPS2OS::AssembleThreadEpilog()
{
	CMIPSAssembler assembler((uint32*)&m_bios[BIOS_ADDRESS_THREADEPILOG - BIOS_ADDRESS_BASE]);

	assembler.ADDIU(CMIPS::V1, CMIPS::R0, 0x23);
	assembler.SYSCALL();
}

void CPS2OS::AssembleIdleThreadProc()
{
	CMIPSAssembler assembler((uint32*)&m_bios[BIOS_ADDRESS_IDLETHREADPROC - BIOS_ADDRESS_BASE]);

	assembler.ADDIU(CMIPS::V1, CMIPS::R0, SYSCALL_CUSTOM_RESCHEDULE);
	assembler.SYSCALL();

	assembler.BEQ(CMIPS::R0, CMIPS::R0, 0xFFFD);
	assembler.NOP();
}

void CPS2OS::AssembleAlarmHandler()
{
	CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_bios[BIOS_ADDRESS_ALARMHANDLER - BIOS_ADDRESS_BASE]));

	auto checkHandlerLabel = assembler.CreateLabel();
	auto moveToNextHandler = assembler.CreateLabel();

	//Prologue
	//S0 -> Handler Counter
	//S1 -> Compare Value
	int32 stackAlloc = 0x20;

	assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
	assembler.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.SD(CMIPS::S1, 0x0010, CMIPS::SP);

	//Load T3 compare
	assembler.LI(CMIPS::S1, CTimer::T3_COMP);
	assembler.LW(CMIPS::S1, 0, CMIPS::S1);

	//Initialize handler loop
	assembler.ADDU(CMIPS::S0, CMIPS::R0, CMIPS::R0);

	assembler.MarkLabel(checkHandlerLabel);

	//Get the address to the current ALARM structure
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(ALARM));
	assembler.MULTU(CMIPS::T0, CMIPS::S0, CMIPS::T0);
	assembler.LI(CMIPS::T1, BIOS_ADDRESS_ALARM_BASE);
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Check validity
	assembler.LW(CMIPS::T1, offsetof(ALARM, isValid), CMIPS::T0);
	assembler.BEQ(CMIPS::T1, CMIPS::R0, moveToNextHandler);
	assembler.NOP();

	assembler.LW(CMIPS::T1, offsetof(ALARM, compare), CMIPS::T0);
	assembler.ORI(CMIPS::T2, CMIPS::R0, 0xFFFF);
	assembler.AND(CMIPS::T1, CMIPS::T1, CMIPS::T2);

	assembler.BNE(CMIPS::T1, CMIPS::S1, moveToNextHandler);
	assembler.NOP();

	//Load the necessary stuff
	assembler.LW(CMIPS::T1, offsetof(ALARM, callback), CMIPS::T0);
	assembler.ADDIU(CMIPS::A0, CMIPS::S0, BIOS_ID_BASE);
	assembler.LW(CMIPS::A1, offsetof(ALARM, delay), CMIPS::T0);
	assembler.LW(CMIPS::A2, offsetof(ALARM, callbackParam), CMIPS::T0);
	assembler.LW(CMIPS::GP, offsetof(ALARM, gp), CMIPS::T0);

	//Jump
	assembler.JALR(CMIPS::T1);
	assembler.NOP();

	//Delete handler (call iReleaseAlarm)
	assembler.ADDIU(CMIPS::A0, CMIPS::S0, BIOS_ID_BASE);
	assembler.ADDIU(CMIPS::V1, CMIPS::R0, -0x1F);
	assembler.SYSCALL();

	assembler.MarkLabel(moveToNextHandler);

	//Increment handler counter and test
	assembler.ADDIU(CMIPS::S0, CMIPS::S0, 0x0001);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, MAX_ALARM - 1);
	assembler.BNE(CMIPS::S0, CMIPS::T0, checkHandlerLabel);
	assembler.NOP();

	//Epilogue
	assembler.LD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.LD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.LD(CMIPS::S1, 0x0010, CMIPS::SP);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);

	assembler.JR(CMIPS::RA);
	assembler.NOP();
}

uint32* CPS2OS::GetCustomSyscallTable()
{
	return (uint32*)&m_ram[BIOS_ADDRESS_CUSTOMSYSCALL_BASE];
}

void CPS2OS::LinkThread(uint32 threadId)
{
	auto thread = m_threads[threadId];

	for(auto threadSchedulePair : m_threadSchedule)
	{
		auto scheduledThread = threadSchedulePair.second;
		if(scheduledThread->currPriority > thread->currPriority)
		{
			m_threadSchedule.AddBefore(threadSchedulePair.first, threadId);
			return;
		}
	}

	m_threadSchedule.PushBack(threadId);
}

void CPS2OS::UnlinkThread(uint32 threadId)
{
	m_threadSchedule.Unlink(threadId);
}

void CPS2OS::ThreadShakeAndBake()
{
	//Don't play with fire (don't switch if we're in exception mode)
	if(m_ee.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL)
	{
		return;
	}

	//Don't switch if interrupts are disabled
	if((m_ee.m_State.nCOP0[CCOP_SCU::STATUS] & INTERRUPTS_ENABLED_MASK) != INTERRUPTS_ENABLED_MASK)
	{
		return;
	}

	//SetupThread has not been called, not ready to switch threads yet
	if(m_currentThreadId == 0)
	{
		return;
	}

	//Select thread to execute
	{
		uint32 nextThreadId = 0;
		if(m_threadSchedule.IsEmpty())
		{
			//No thread ready to be executed
			nextThreadId = m_idleThreadId;
		}
		else
		{
			auto nextThreadPair = *m_threadSchedule.begin();
			nextThreadId = nextThreadPair.first;
			assert(nextThreadPair.second->status == THREAD_RUNNING);
		}
		ThreadSwitchContext(nextThreadId);
	}
}

void CPS2OS::ThreadSwitchContext(uint32 id)
{
	if(id == m_currentThreadId) return;

	//Save the context of the current thread
	{
		auto thread = m_threads[m_currentThreadId];
		assert(thread);
		thread->epc = m_ee.m_State.nPC;

		//Idle thread doesn't have a context
		if(m_currentThreadId != m_idleThreadId)
		{
			ThreadSaveContext(thread, false);
		}
	}

	m_currentThreadId = id;
	m_idleEvaluator.NotifyEvent(Ee::CIdleEvaluator::EVENT_CHANGETHREAD, id);

	//Load the new context
	{
		auto thread = m_threads[m_currentThreadId];
		assert(thread);
		m_ee.m_State.nPC = thread->epc;

		//Idle thread doesn't have a context
		if(id != m_idleThreadId)
		{
			ThreadLoadContext(thread, false);
		}
	}

	CLog::GetInstance().Print(LOG_NAME, "New thread elected (id = %i).\r\n", id);
}

void CPS2OS::ThreadSaveContext(THREAD* thread, bool interrupt)
{
	if(interrupt)
	{
		thread->contextPtr = BIOS_ADDRESS_INTERRUPT_THREAD_CONTEXT;
	}
	else
	{
		thread->contextPtr = m_ee.m_State.nGPR[CMIPS::SP].nV0 - STACKRES;
	}

	auto context = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));

	//Save the context
	for(uint32 i = 0; i < 0x20; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		context->gpr[i] = m_ee.m_State.nGPR[i];
	}
	for(uint32 i = 0; i < 0x20; i++)
	{
		context->cop1[i] = m_ee.m_State.nCOP1[i];
	}
	auto& sa = context->gpr[CMIPS::R0].nV0;
	auto& hi = context->gpr[CMIPS::K0];
	auto& lo = context->gpr[CMIPS::K1];
	sa = m_ee.m_State.nSA >> 3; //Act as if MFSA was used
	hi.nV[0] = m_ee.m_State.nHI[0];
	hi.nV[1] = m_ee.m_State.nHI[1];
	hi.nV[2] = m_ee.m_State.nHI1[0];
	hi.nV[3] = m_ee.m_State.nHI1[1];
	lo.nV[0] = m_ee.m_State.nLO[0];
	lo.nV[1] = m_ee.m_State.nLO[1];
	lo.nV[2] = m_ee.m_State.nLO1[0];
	lo.nV[3] = m_ee.m_State.nLO1[1];
	context->cop1a = m_ee.m_State.nCOP1A;
	context->fcsr = m_ee.m_State.nFCSR;
}

void CPS2OS::ThreadLoadContext(THREAD* thread, bool interrupt)
{
	assert(thread->contextPtr != 0);
	assert(!interrupt || (thread->contextPtr == BIOS_ADDRESS_INTERRUPT_THREAD_CONTEXT));

	auto context = reinterpret_cast<const THREADCONTEXT*>(GetStructPtr(thread->contextPtr));
	for(uint32 i = 0; i < 0x20; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		m_ee.m_State.nGPR[i] = context->gpr[i];
	}
	for(uint32 i = 0; i < 0x20; i++)
	{
		m_ee.m_State.nCOP1[i] = context->cop1[i];
	}
	auto& sa = context->gpr[CMIPS::R0].nV0;
	auto& hi = context->gpr[CMIPS::K0];
	auto& lo = context->gpr[CMIPS::K1];
	m_ee.m_State.nSA = (sa & 0x0F) << 3; //Act as if MTSA was used
	m_ee.m_State.nHI[0] = hi.nV[0];
	m_ee.m_State.nHI[1] = hi.nV[1];
	m_ee.m_State.nHI1[0] = hi.nV[2];
	m_ee.m_State.nHI1[1] = hi.nV[3];
	m_ee.m_State.nLO[0] = lo.nV[0];
	m_ee.m_State.nLO[1] = lo.nV[1];
	m_ee.m_State.nLO1[0] = lo.nV[2];
	m_ee.m_State.nLO1[1] = lo.nV[3];
	m_ee.m_State.nCOP1A = context->cop1a;
	m_ee.m_State.nFCSR = context->fcsr;
}

void CPS2OS::ThreadReset(uint32 id)
{
	assert(m_currentThreadId != id);

	auto thread = m_threads[id];
	assert(thread->status == THREAD_ZOMBIE);

	uint32 stackTop = thread->stackBase + thread->stackSize;

	thread->contextPtr = stackTop - STACKRES;
	thread->currPriority = thread->initPriority;
	//Reset wakeup count?

	auto context = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));
	context->gpr[CMIPS::SP].nV0 = stackTop - STACK_FRAME_RESERVE_SIZE;
	context->gpr[CMIPS::FP].nV0 = stackTop - STACK_FRAME_RESERVE_SIZE;
	context->gpr[CMIPS::GP].nV0 = thread->gp;
	context->gpr[CMIPS::RA].nV0 = BIOS_ADDRESS_THREADEPILOG;
}

void CPS2OS::CheckLivingThreads()
{
	//Check if we have a living thread (this is needed for autotests to work properly)
	bool hasLiveThread = false;
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->status != THREAD_ZOMBIE)
		{
			hasLiveThread = true;
			break;
		}
	}

	if(!hasLiveThread)
	{
		OnRequestExit();
	}
}

void CPS2OS::SemaLinkThread(uint32 semaId, uint32 threadId)
{
	auto thread = m_threads[threadId];
	auto sema = m_semaphores[semaId];

	assert(sema);

	assert(thread);
	assert(thread->semaWait == 0);
	assert(thread->nextId == 0);

	uint32* waitNextId = &sema->waitNextId;
	while((*waitNextId) != 0)
	{
		auto waitThread = m_threads[*waitNextId];
		waitNextId = &waitThread->nextId;
	}

	(*waitNextId) = threadId;
	thread->semaWait = semaId;

	sema->waitCount++;
}

void CPS2OS::SemaUnlinkThread(uint32 semaId, uint32 threadId)
{
	auto thread = m_threads[threadId];
	auto sema = m_semaphores[semaId];

	assert(sema);
	assert(sema->waitCount != 0);
	assert(sema->waitNextId != 0);

	assert(thread);
	assert(thread->semaWait == semaId);

	uint32* waitNextId = &sema->waitNextId;
	while((*waitNextId) != 0)
	{
		if((*waitNextId) == threadId)
		{
			break;
		}
		auto waitThread = m_threads[*waitNextId];
		waitNextId = &waitThread->nextId;
	}

	assert((*waitNextId) != 0);
	(*waitNextId) = thread->nextId;
	thread->nextId = 0;
	thread->semaWait = 0;

	sema->waitCount--;
}

void CPS2OS::SemaReleaseSingleThread(uint32 semaId, bool cancelled)
{
	//Releases a single thread from a semaphore's queue

	auto sema = m_semaphores[semaId];
	assert(sema);
	assert(sema->waitCount != 0);
	assert(sema->waitNextId != 0);

	uint32 threadId = sema->waitNextId;

	auto thread = m_threads[threadId];
	assert(thread);
	assert(thread->semaWait == semaId);

	//Unlink thread
	sema->waitNextId = thread->nextId;
	thread->nextId = 0;
	thread->semaWait = 0;

	switch(thread->status)
	{
	case THREAD_WAITING:
		thread->status = THREAD_RUNNING;
		LinkThread(threadId);
		break;
	case THREAD_SUSPENDED_WAITING:
		thread->status = THREAD_SUSPENDED;
		break;
	default:
		//Invalid thread state
		assert(0);
		break;
	}

	uint32 returnValue = cancelled ? -1 : semaId;
	auto context = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));
	context->gpr[SC_RETURN].nD0 = static_cast<int32>(returnValue);

	sema->waitCount--;
}

void CPS2OS::AlarmUpdateCompare()
{
	uint32 minCompare = UINT32_MAX;
	for(const auto& alarm : m_alarms)
	{
		if(!alarm) continue;
		minCompare = std::min<uint32>(alarm->compare, minCompare);
	}

	if(minCompare == UINT32_MAX)
	{
		//No alarm to watch
		return;
	}

	//Enable TIMER3 INTs
	m_ee.m_pMemoryMap->SetWord(CTimer::T3_MODE, CTimer::MODE_CLOCK_SELECT_EXTERNAL | CTimer::MODE_COUNT_ENABLE | CTimer::MODE_EQUAL_FLAG | CTimer::MODE_EQUAL_INT_ENABLE);
	m_ee.m_pMemoryMap->SetWord(CTimer::T3_COMP, minCompare & 0xFFFF);

	uint32 mask = (1 << CINTC::INTC_LINE_TIMER3);
	if(!(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & mask))
	{
		m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, mask);
	}
}

void CPS2OS::CreateIdleThread()
{
	m_idleThreadId = m_threads.AllocateAt(m_state->allocateThreadNextId);
	auto thread = m_threads[m_idleThreadId];
	thread->epc = BIOS_ADDRESS_IDLETHREADPROC;
	thread->status = THREAD_ZOMBIE;
}

std::pair<uint32, uint32> CPS2OS::GetVsyncFlagPtrs() const
{
	return std::make_pair(m_state->vsyncFlagValue1Ptr, m_state->vsyncFlagValue2Ptr);
}

void CPS2OS::SetVsyncFlagPtrs(uint32 value1Ptr, uint32 value2Ptr)
{
	m_state->vsyncFlagValue1Ptr = value1Ptr;
	m_state->vsyncFlagValue2Ptr = value2Ptr;
}

uint8* CPS2OS::GetStructPtr(uint32 address) const
{
	address = TranslateAddress(nullptr, address);
	uint8* memory = nullptr;
	if((address >= PS2::EE_SPR_ADDR) && (address < (PS2::EE_SPR_ADDR + PS2::EE_SPR_SIZE)))
	{
		address &= (PS2::EE_SPR_SIZE - 1);
		memory = m_spr;
	}
	else
	{
		address &= (m_ramSize - 1);
		memory = m_ram;
	}
	return memory + address;
}

uint32 CPS2OS::GetNextAvailableDeci2HandlerId()
{
	for(uint32 i = 1; i < MAX_DECI2HANDLER; i++)
	{
		DECI2HANDLER* handler = GetDeci2Handler(i);
		if(handler->valid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::DECI2HANDLER* CPS2OS::GetDeci2Handler(uint32 id)
{
	id--;
	return &((DECI2HANDLER*)&m_ram[0x00008000])[id];
}

Ee::CLibMc2& CPS2OS::GetLibMc2()
{
	return m_libMc2;
}

void CPS2OS::HandleInterrupt(int32 cpuIntLine)
{
	//Check if interrupts are enabled here because EIE bit isn't checked by CMIPS
	if((m_ee.m_State.nCOP0[CCOP_SCU::STATUS] & INTERRUPTS_ENABLED_MASK) != INTERRUPTS_ENABLED_MASK)
	{
		return;
	}

	if(!m_ee.CanGenerateInterrupt())
	{
		return;
	}

	if(m_currentThreadId != m_idleThreadId)
	{
		auto thread = m_threads[m_currentThreadId];
		ThreadSaveContext(thread, true);

		m_idleEvaluator.NotifyEvent(Ee::CIdleEvaluator::EVENT_INTERRUPT, 0);
	}

	//Update CAUSE register
	m_ee.m_State.nCOP0[CCOP_SCU::CAUSE] &= ~(CCOP_SCU::CAUSE_EXCCODE_MASK | CCOP_SCU::CAUSE_IP_2 | CCOP_SCU::CAUSE_IP_3);
	m_ee.m_State.nCOP0[CCOP_SCU::CAUSE] |= CCOP_SCU::CAUSE_EXCCODE_INT;

	switch(cpuIntLine)
	{
	case 0:
		//INTC interrupt
		m_ee.m_State.nCOP0[CCOP_SCU::CAUSE] |= CCOP_SCU::CAUSE_IP_2;
		break;
	case 1:
		//DMAC interrupt
		m_ee.m_State.nCOP0[CCOP_SCU::CAUSE] |= CCOP_SCU::CAUSE_IP_3;
		break;
	default:
		assert(false);
		break;
	}

	FRAMEWORK_MAYBE_UNUSED bool interrupted = m_ee.GenerateInterrupt(BIOS_ADDRESS_INTERRUPTHANDLER);
	assert(interrupted);
}

void CPS2OS::HandleReturnFromException()
{
	ThreadShakeAndBake();
}

bool CPS2OS::CheckVBlankFlag()
{
	bool changed = false;
	auto vsyncFlagPtrs = GetVsyncFlagPtrs();

	if(vsyncFlagPtrs.first != 0)
	{
		auto vsyncFlagFirst = GetStructPtr(vsyncFlagPtrs.first);
		*reinterpret_cast<uint32*>(vsyncFlagFirst) = 1;
		changed = true;
	}
	if(vsyncFlagPtrs.second != 0)
	{
		auto vsyncFlagSecond = GetStructPtr(vsyncFlagPtrs.second);
		*reinterpret_cast<uint64*>(vsyncFlagSecond) = m_gs->ReadPrivRegister(CGSHandler::GS_CSR);
		changed = true;
	}

	SetVsyncFlagPtrs(0, 0);
	return changed;
}

uint32 CPS2OS::SuspendCurrentThread()
{
	//For HLE module usage

	uint32 threadId = m_currentThreadId;

	auto thread = m_threads[threadId];
	assert(thread);
	assert(thread->status == THREAD_RUNNING);

	thread->status = THREAD_SUSPENDED;
	UnlinkThread(threadId);

	ThreadShakeAndBake();

	return threadId;
}

void CPS2OS::ResumeThread(uint32 threadId)
{
	//For HLE module usage

	auto thread = m_threads[threadId];
	assert(thread);
	assert(thread->status == THREAD_SUSPENDED);

	thread->status = THREAD_RUNNING;
	LinkThread(threadId);
}

void CPS2OS::UpdateTLBEnabledState()
{
	bool TLBenabled = (m_tlblExceptionHandler != 0) || (m_tlbsExceptionHandler != 0);
	if(TLBenabled)
	{
		m_ee.m_pAddrTranslator = &TranslateAddressTLB;
		m_ee.m_TLBExceptionChecker = &CheckTLBExceptions;
	}
	else
	{
		m_ee.m_pAddrTranslator = &TranslateAddress;
		m_ee.m_TLBExceptionChecker = nullptr;
	}
}

//The EE kernel defines some default TLB entries
//These defines seek to allow simple translation without scanning the TLB

//Direct access to physical addresses
#define TLB_IS_DIRECT_VADDRESS(a) ((a) <= 0x1FFFFFFF)
#define TLB_TRANSLATE_DIRECT_VADDRESS(a) ((a)&0x1FFFFFFF)

//RAM access, "uncached" as per TLB entry
#define TLB_IS_UNCACHED_RAM_VADDRESS(a) (((a) >= 0x20100000) && ((a) <= 0x21FFFFFF))
#define TLB_TRANSLATE_UNCACHED_RAM_VADDRESS(a) ((a)-0x20000000)

//RAM access, "uncached & accelerated" as per TLB entry
#define TLB_IS_UNCACHED_FAST_RAM_VADDRESS(a) (((a) >= 0x30100000) && ((a) <= 0x31FFFFFF))
#define TLB_TRANSLATE_UNCACHED_FAST_RAM_VADDRESS(a) ((a)-0x30000000)

//SPR memory access, the TLB entry for this should have the SPR bit set
#define TLB_IS_SPR_VADDRESS(a) (((a) >= 0x70000000) && ((a) <= 0x70003FFF))
#define TLB_TRANSLATE_SPR_VADDRESS(a) ((a) - (0x70000000 - PS2::EE_SPR_ADDR))

uint32 CPS2OS::TranslateAddress(CMIPS*, uint32 vaddrLo)
{
	if(TLB_IS_SPR_VADDRESS(vaddrLo))
	{
		return TLB_TRANSLATE_SPR_VADDRESS(vaddrLo);
	}
	if(TLB_IS_UNCACHED_FAST_RAM_VADDRESS(vaddrLo))
	{
		return TLB_TRANSLATE_UNCACHED_FAST_RAM_VADDRESS(vaddrLo);
	}
	//No need to check "uncached" RAM access here, will be translated correctly by the macro below
	return TLB_TRANSLATE_DIRECT_VADDRESS(vaddrLo);
}

uint32 CPS2OS::TranslateAddressTLB(CMIPS* context, uint32 vaddrLo)
{
	if(TLB_IS_DIRECT_VADDRESS(vaddrLo))
	{
		return TLB_TRANSLATE_DIRECT_VADDRESS(vaddrLo);
	}
	if(TLB_IS_UNCACHED_RAM_VADDRESS(vaddrLo))
	{
		return TLB_TRANSLATE_UNCACHED_RAM_VADDRESS(vaddrLo);
	}
	if(TLB_IS_UNCACHED_FAST_RAM_VADDRESS(vaddrLo))
	{
		return TLB_TRANSLATE_UNCACHED_FAST_RAM_VADDRESS(vaddrLo);
	}
	if(TLB_IS_SPR_VADDRESS(vaddrLo))
	{
		return TLB_TRANSLATE_SPR_VADDRESS(vaddrLo);
	}
	for(uint32 i = 0; i < MIPSSTATE::TLB_ENTRY_MAX; i++)
	{
		const auto& entry = context->m_State.tlbEntries[i];
		if(entry.entryHi == 0) continue;

		uint32 pageSize = ((entry.pageMask >> 13) + 1) * 0x1000;
		uint32 vpnMask = (pageSize * 2) - 1;
		uint32 vpn = vaddrLo & ~vpnMask;
		uint32 tlbVpn = entry.entryHi & ~vpnMask;

		if(vpn == tlbVpn)
		{
			uint32 mask = (pageSize - 1);
			uint32 entryLo = (vaddrLo & pageSize) ? entry.entryLo1 : entry.entryLo0;
			assert(entryLo & CCOP_SCU::ENTRYLO_GLOBAL);
			assert(entryLo & CCOP_SCU::ENTRYLO_VALID);
			uint32 pfn = ((entryLo >> 6) << 12);
			uint32 paddr = pfn + (vaddrLo & mask);
			return paddr;
		}
	}
	//Shouldn't come here
	//assert(false);
	return vaddrLo & 0x1FFFFFFF;
}

uint32 CPS2OS::CheckTLBExceptions(CMIPS* context, uint32 vaddrLo, uint32 isWrite)
{
	if(TLB_IS_DIRECT_VADDRESS(vaddrLo))
	{
		return MIPS_EXCEPTION_NONE;
	}
	if(TLB_IS_UNCACHED_RAM_VADDRESS(vaddrLo))
	{
		return MIPS_EXCEPTION_NONE;
	}
	if(TLB_IS_UNCACHED_FAST_RAM_VADDRESS(vaddrLo))
	{
		return MIPS_EXCEPTION_NONE;
	}
	if(TLB_IS_SPR_VADDRESS(vaddrLo))
	{
		return MIPS_EXCEPTION_NONE;
	}
	for(uint32 i = 0; i < MIPSSTATE::TLB_ENTRY_MAX; i++)
	{
		const auto& entry = context->m_State.tlbEntries[i];
		if(entry.entryHi == 0) continue;

		uint32 pageSize = ((entry.pageMask >> 13) + 1) * 0x1000;
		uint32 vpnMask = (pageSize * 2) - 1;
		uint32 vpn = vaddrLo & ~vpnMask;
		uint32 tlbVpn = entry.entryHi & ~vpnMask;

		if(vpn == tlbVpn)
		{
			uint32 entryLo = (vaddrLo & pageSize) ? entry.entryLo1 : entry.entryLo0;
			assert(entryLo & CCOP_SCU::ENTRYLO_GLOBAL);
			if((entryLo & CCOP_SCU::ENTRYLO_VALID) == 0)
			{
				//This causes a TLBL or TLBS exception, but should use the common vector
				context->m_State.nCOP0[CCOP_SCU::CAUSE] &= ~CCOP_SCU::CAUSE_EXCCODE_MASK;
				context->m_State.nCOP0[CCOP_SCU::CAUSE] |= isWrite ? CCOP_SCU::CAUSE_EXCCODE_TLBS : CCOP_SCU::CAUSE_EXCCODE_TLBL;
				context->m_State.nCOP0[CCOP_SCU::BADVADDR] = vaddrLo;
				context->m_State.nHasException = MIPS_EXCEPTION_TLB;
				return context->m_State.nHasException;
			}
			assert(!isWrite || ((entryLo & CCOP_SCU::ENTRYLO_DIRTY) != 0));
			return MIPS_EXCEPTION_NONE;
		}
	}
	//assert(false);
	//We should probably raise some kind of exception here since we didn't match anything
	return MIPS_EXCEPTION_NONE;
}

//////////////////////////////////////////////////
//System Calls
//////////////////////////////////////////////////

void CPS2OS::sc_Unhandled()
{
	CLog::GetInstance().Warn(LOG_NAME, "Unknown system call (0x%X) called from 0x%08X.\r\n", m_ee.m_State.nGPR[3].nV[0], m_ee.m_State.nPC);
}

//02
void CPS2OS::sc_GsSetCrt()
{
	bool isInterlaced = (m_ee.m_State.nGPR[SC_PARAM0].nV[0] != 0);
	unsigned int mode = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	bool isFrameMode = (m_ee.m_State.nGPR[SC_PARAM2].nV[0] != 0);

	if(m_gs != NULL)
	{
		m_gs->SetCrt(isInterlaced, mode, isFrameMode);
	}

	OnCrtModeChange();
}

//04
void CPS2OS::sc_Exit()
{
	OnRequestExit();
}

//06
void CPS2OS::sc_LoadExecPS2()
{
	uint32 filePathPtr = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 argCount = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 argValuesPtr = m_ee.m_State.nGPR[SC_PARAM2].nV[0];

	ArgumentList arguments;
	for(uint32 i = 0; i < argCount; i++)
	{
		uint32 argValuePtr = *reinterpret_cast<uint32*>(GetStructPtr(argValuesPtr + i * 4));
		arguments.push_back(reinterpret_cast<const char*>(GetStructPtr(argValuePtr)));
	}

	std::string filePath = reinterpret_cast<const char*>(GetStructPtr(filePathPtr));
	if(filePath.find(':') == std::string::npos)
	{
		//Unreal Tournament doesn't provide drive info in path
		filePath = "cdrom0:" + filePath;
	}
	OnRequestLoadExecutable(filePath.c_str(), arguments);
}

//07
void CPS2OS::sc_ExecPS2()
{
	uint32 pc = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 gp = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 argCount = m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	uint32 argValuesPtr = m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	//This is used by Gran Turismo 4. The game clears the program
	//loaded initially, so, we need to make sure there's no stale
	//threads or handlers

	sc_ExitDeleteThread();
	assert(m_currentThreadId == m_idleThreadId);

	//Clear all remaining INTC handlers
	std::vector<uint32> intcHandlerIds;
	for(const auto& intcHandlerPair : m_intcHandlerQueue)
	{
		intcHandlerIds.push_back(intcHandlerPair.first);
	}
	for(const auto& intcHandlerId : intcHandlerIds)
	{
		m_intcHandlerQueue.Unlink(intcHandlerId);
		m_intcHandlers.Free(intcHandlerId);
	}
	//Also make sure all interrupts are masked
	{
		uint32 intcMask = m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK);
		m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, intcMask);
	}

	m_ee.m_State.nPC = pc;
	m_ee.m_State.nGPR[CMIPS::GP].nD0 = static_cast<int32>(gp);

	ArgumentList arguments;
	for(uint32 i = 0; i < argCount; i++)
	{
		uint32 argValuePtr = *reinterpret_cast<uint32*>(GetStructPtr(argValuesPtr + i * 4));
		arguments.push_back(reinterpret_cast<const char*>(GetStructPtr(argValuePtr)));
	}
	m_currentArguments = arguments;
}

//0D
void CPS2OS::sc_SetVTLBRefillHandler()
{
	uint32 cause = m_ee.m_State.nGPR[SC_PARAM0].nV0;
	uint32 handler = m_ee.m_State.nGPR[SC_PARAM1].nV0;

	switch(cause << 2)
	{
	case CCOP_SCU::CAUSE_EXCCODE_TLBL:
		m_tlblExceptionHandler = handler;
		break;
	case CCOP_SCU::CAUSE_EXCCODE_TLBS:
		m_tlbsExceptionHandler = handler;
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Setting handler 0x%08X for unknown exception %d.\r\n", handler, cause);
		break;
	}

	UpdateTLBEnabledState();

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(handler);
}

//0E
void CPS2OS::sc_SetVCommonHandler()
{
	uint32 cause = m_ee.m_State.nGPR[SC_PARAM0].nV0;
	uint32 handler = m_ee.m_State.nGPR[SC_PARAM1].nV0;

	switch(cause << 2)
	{
	case CCOP_SCU::CAUSE_EXCCODE_TRAP:
		m_trapExceptionHandler = handler;
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Setting handler 0x%08X for unknown exception %d.\r\n", handler, cause);
		break;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(handler);
}

//10
void CPS2OS::sc_AddIntcHandler()
{
	uint32 cause = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 address = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 next = m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	uint32 arg = m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	uint32 id = m_intcHandlers.Allocate();
	if(static_cast<int32>(id) == -1)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto handler = m_intcHandlers[id];
	handler->address = address;
	handler->cause = cause;
	handler->arg = arg;
	handler->gp = m_ee.m_State.nGPR[CMIPS::GP].nV[0];

	if(next == 0)
	{
		m_intcHandlerQueue.PushFront(id);
	}
	else if(static_cast<int32>(next) == -1)
	{
		m_intcHandlerQueue.PushBack(id);
	}
	else
	{
		m_intcHandlerQueue.AddBefore(next, id);
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
}

//11
void CPS2OS::sc_RemoveIntcHandler()
{
	uint32 cause = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 id = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	auto handler = m_intcHandlers[id];
	if(!handler)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}
	assert(handler->cause == cause);

	m_intcHandlerQueue.Unlink(id);
	m_intcHandlers.Free(id);

	int32 handlerCount = 0;
	for(const auto& handler : m_intcHandlers)
	{
		if(!handler) continue;
		if(handler->cause != cause) continue;
		handlerCount++;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = handlerCount;
}

//12
void CPS2OS::sc_AddDmacHandler()
{
	uint32 channel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 address = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 next = m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	uint32 arg = m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	uint32 id = m_dmacHandlers.Allocate();
	if(static_cast<int32>(id) == -1)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto handler = m_dmacHandlers[id];
	handler->address = address;
	handler->channel = channel;
	handler->arg = arg;
	handler->gp = m_ee.m_State.nGPR[CMIPS::GP].nV[0];

	if(next == 0)
	{
		m_dmacHandlerQueue.PushFront(id);
	}
	else if(static_cast<int32>(next) == -1)
	{
		m_dmacHandlerQueue.PushBack(id);
	}
	else
	{
		m_dmacHandlerQueue.AddBefore(next, id);
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//13
void CPS2OS::sc_RemoveDmacHandler()
{
	uint32 channel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 id = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	auto handler = m_dmacHandlers[id];
	if(!handler)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}
	assert(handler->channel == channel);

	m_dmacHandlerQueue.Unlink(id);
	m_dmacHandlers.Free(id);

	int32 handlerCount = 0;
	for(const auto& handler : m_dmacHandlers)
	{
		if(!handler) continue;
		if(handler->channel != channel) continue;
		handlerCount++;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = handlerCount;
}

//14
//1A
void CPS2OS::sc_EnableIntc()
{
	uint32 cause = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 mask = 1 << cause;
	bool changed = false;

	if(!(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & mask))
	{
		m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, mask);
		changed = true;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = changed ? 1 : 0;
}

//15
//1B
void CPS2OS::sc_DisableIntc()
{
	uint32 cause = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 mask = 1 << cause;
	bool changed = false;

	if(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & mask)
	{
		m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, mask);
		changed = true;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = changed ? 1 : 0;
}

//16
//1C
void CPS2OS::sc_EnableDmac()
{
	uint32 channel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 registerId = 0x10000 << channel;
	bool changed = false;

	if(!(m_ee.m_pMemoryMap->GetWord(CDMAC::D_STAT) & registerId))
	{
		m_ee.m_pMemoryMap->SetWord(CDMAC::D_STAT, registerId);
		changed = true;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = changed ? 1 : 0;
}

//17
//1D
void CPS2OS::sc_DisableDmac()
{
	uint32 channel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 registerId = 0x10000 << channel;
	bool changed = false;

	if(m_ee.m_pMemoryMap->GetWord(CDMAC::D_STAT) & registerId)
	{
		m_ee.m_pMemoryMap->SetWord(CDMAC::D_STAT, registerId);
		changed = true;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = changed ? 1 : 0;
}

//18/1E
void CPS2OS::sc_SetAlarm()
{
	uint32 delay = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 callback = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 callbackParam = m_ee.m_State.nGPR[SC_PARAM2].nV[0];

	auto alarmId = m_alarms.Allocate();
	assert(alarmId != -1);
	if(alarmId == -1)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	//Check limits (0 delay will probably be problematic and delay is a short)
	assert((delay > 0) && (delay < 0x10000));

	uint32 currentCount = m_ee.m_pMemoryMap->GetWord(CTimer::T3_COUNT);

	auto alarm = m_alarms[alarmId];
	alarm->delay = delay;
	alarm->compare = currentCount + delay;
	alarm->callback = callback;
	alarm->callbackParam = callbackParam;
	alarm->gp = m_ee.m_State.nGPR[CMIPS::GP].nV0;

	AlarmUpdateCompare();

	m_ee.m_State.nGPR[SC_RETURN].nD0 = alarmId;
}

//1F
void CPS2OS::sc_ReleaseAlarm()
{
	uint32 alarmId = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	auto alarm = m_alarms[alarmId];
	if(alarm == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	m_alarms.Free(alarmId);

	AlarmUpdateCompare();
}

//20
void CPS2OS::sc_CreateThread()
{
	auto threadParam = reinterpret_cast<THREADPARAM*>(GetStructPtr(m_ee.m_State.nGPR[SC_PARAM0].nV0));

	uint32 id = m_threads.AllocateAt(m_state->allocateThreadNextId);
	if(static_cast<int32>(id) == -1)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
		return;
	}

	auto parentThread = m_threads[m_currentThreadId];
	assert(parentThread);
	uint32 heapBase = parentThread->heapBase;

	assert(threadParam->initPriority < 128);

	auto thread = m_threads[id];
	thread->status = THREAD_ZOMBIE;
	thread->stackBase = threadParam->stackBase;
	thread->epc = threadParam->threadProc;
	thread->threadProc = threadParam->threadProc;
	thread->initPriority = threadParam->initPriority;
	thread->heapBase = heapBase;
	thread->wakeUpCount = 0;
	thread->gp = threadParam->gp;
	thread->stackSize = threadParam->stackSize;

	ThreadReset(id);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
}

//21
void CPS2OS::sc_DeleteThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	if(id == m_currentThreadId)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	//Prevent deletion of idle thread.
	//Taiko no Tatsujin 14 tries to delete all threads (including this one) before starting a new EE exec
	if(id == m_idleThreadId)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(id >= MAX_THREAD)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(thread->status != THREAD_ZOMBIE)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	m_threads.Free(id);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
}

//22
void CPS2OS::sc_StartThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 arg = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	assert(thread->status == THREAD_ZOMBIE);
	thread->status = THREAD_RUNNING;
	thread->epc = thread->threadProc;

	auto context = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));
	context->gpr[CMIPS::A0].nV0 = arg;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);

	LinkThread(id);
	ThreadShakeAndBake();
}

//23
void CPS2OS::sc_ExitThread()
{
	uint32 threadId = m_currentThreadId;

	auto thread = m_threads[threadId];
	thread->status = THREAD_ZOMBIE;
	UnlinkThread(threadId);

	ThreadShakeAndBake();
	ThreadReset(threadId);

	CheckLivingThreads();
}

//24
void CPS2OS::sc_ExitDeleteThread()
{
	uint32 threadId = m_currentThreadId;

	auto thread = m_threads[threadId];
	thread->status = THREAD_ZOMBIE;
	UnlinkThread(threadId);

	ThreadShakeAndBake();

	m_threads.Free(threadId);

	CheckLivingThreads();
}

//25
void CPS2OS::sc_TerminateThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	if(id == m_currentThreadId)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(thread->status == THREAD_ZOMBIE)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	switch(thread->status)
	{
	case THREAD_RUNNING:
		UnlinkThread(id);
		break;
	case THREAD_WAITING:
	case THREAD_SUSPENDED_WAITING:
		SemaUnlinkThread(thread->semaWait, id);
		break;
	default:
		assert(0);
		[[fallthrough]];
	case THREAD_SLEEPING:
	case THREAD_SUSPENDED:
	case THREAD_SUSPENDED_SLEEPING:
		//Nothing to do
		break;
	}

	thread->status = THREAD_ZOMBIE;
	ThreadReset(id);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
}

//29
//2A
void CPS2OS::sc_ChangeThreadPriority()
{
	bool isInt = m_ee.m_State.nGPR[3].nV[0] == 0x2A;
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 prio = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	uint32 prevPrio = thread->currPriority;
	thread->currPriority = prio;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(prevPrio);

	//Reschedule if thread is already running
	if(thread->status == THREAD_RUNNING)
	{
		UnlinkThread(id);
		LinkThread(id);
	}

	if(!isInt)
	{
		ThreadShakeAndBake();
	}
}

//2B
void CPS2OS::sc_RotateThreadReadyQueue()
{
	uint32 prio = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	uint32 threadIdBefore = m_currentThreadId;

	//Find first of this priority and reinsert if it's the same as the current thread
	//If it's not the same, the schedule will be rotated when another thread is choosen
	for(auto threadSchedulePair : m_threadSchedule)
	{
		auto scheduledThread = threadSchedulePair.second;
		if(scheduledThread->currPriority == prio)
		{
			UnlinkThread(threadSchedulePair.first);
			LinkThread(threadSchedulePair.first);
			break;
		}
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(prio);

	ThreadShakeAndBake();

	m_idleEvaluator.NotifyEvent(Ee::CIdleEvaluator::EVENT_ROTATETHREADREADYQUEUE, threadIdBefore, m_currentThreadId);
}

//2D/2E
void CPS2OS::sc_ReleaseWaitThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	bool isInt = m_ee.m_State.nGPR[3].nV[0] == 0x2E;

	auto thread = m_threads[id];
	if(!thread)
	{
		CLog::GetInstance().Warn(LOG_NAME, SYSCALL_NAME_RELEASEWAITTHREAD ": Used on thread that doesn't exist (%d).\r\n",
		                         id);
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	//Must also work with THREAD_WAITING
	assert(thread->status != THREAD_WAITING);
	if(thread->status != THREAD_SLEEPING)
	{
		CLog::GetInstance().Warn(LOG_NAME, SYSCALL_NAME_RELEASEWAITTHREAD ": Used on non sleeping thread (%d).\r\n",
		                         id);
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	//TODO: Check what happens to wakeUpCount
	thread->wakeUpCount = 0;
	thread->status = THREAD_RUNNING;
	LinkThread(id);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);

	if(!isInt)
	{
		ThreadShakeAndBake();
	}
}

//2F
void CPS2OS::sc_GetThreadId()
{
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = m_currentThreadId;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//30
void CPS2OS::sc_ReferThreadStatus()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 statusPtr = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	if(id >= MAX_THREAD)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(id == 0)
	{
		id = m_currentThreadId;
	}

	auto thread = m_threads[id];
	if(!thread)
	{
		//Returns 0 if thread has been deleted (needed by MGS2)
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(0);
		return;
	}

	uint32 ret = 0;
	switch(thread->status)
	{
	case THREAD_RUNNING:
		if(id == m_currentThreadId)
		{
			ret = THS_RUN;
		}
		else
		{
			ret = THS_READY;
		}
		break;
	case THREAD_WAITING:
	case THREAD_SLEEPING:
		ret = THS_WAIT;
		break;
	case THREAD_SUSPENDED:
		ret = THS_SUSPEND;
		break;
	case THREAD_SUSPENDED_WAITING:
	case THREAD_SUSPENDED_SLEEPING:
		ret = (THS_WAIT | THS_SUSPEND);
		break;
	case THREAD_ZOMBIE:
		ret = THS_DORMANT;
		break;
	}

	uint32 waitType = 0;
	switch(thread->status)
	{
	case THREAD_SLEEPING:
	case THREAD_SUSPENDED_SLEEPING:
		waitType = 1;
		break;
	case THREAD_WAITING:
	case THREAD_SUSPENDED_WAITING:
		waitType = 2;
		break;
	default:
		waitType = 0;
		break;
	}

	if(statusPtr != 0)
	{
		auto status = reinterpret_cast<THREADSTATUS*>(GetStructPtr(statusPtr));

		status->status = ret;
		status->initPriority = thread->initPriority;
		status->currPriority = thread->currPriority;
		status->stackBase = thread->stackBase;
		status->stackSize = thread->stackSize;
		status->waitType = waitType;
		status->wakeupCount = thread->wakeUpCount;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = ret;
}

//32
void CPS2OS::sc_SleepThread()
{
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(m_currentThreadId);

	auto thread = m_threads[m_currentThreadId];
	if(thread->wakeUpCount == 0)
	{
		assert(thread->status == THREAD_RUNNING);
		thread->status = THREAD_SLEEPING;
		UnlinkThread(m_currentThreadId);
		ThreadShakeAndBake();
		return;
	}

	thread->wakeUpCount--;
}

//33
//34
void CPS2OS::sc_WakeupThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	bool isInt = m_ee.m_State.nGPR[3].nV[0] == 0x34;

	if((id == 0) || (id == m_currentThreadId))
	{
		//Can't wakeup a thread that's already running
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(thread->status == THREAD_ZOMBIE)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);

	if(
	    (thread->status == THREAD_SLEEPING) ||
	    (thread->status == THREAD_SUSPENDED_SLEEPING))
	{
		switch(thread->status)
		{
		case THREAD_SLEEPING:
			thread->status = THREAD_RUNNING;
			LinkThread(id);
			break;
		case THREAD_SUSPENDED_SLEEPING:
			thread->status = THREAD_SUSPENDED;
			break;
		default:
			assert(0);
			break;
		}
		if(!isInt)
		{
			ThreadShakeAndBake();
		}
	}
	else
	{
		thread->wakeUpCount++;
	}
}

//35
//36
void CPS2OS::sc_CancelWakeupThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	FRAMEWORK_MAYBE_UNUSED bool isInt = m_ee.m_State.nGPR[3].nV[0] == 0x36;

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	uint32 wakeUpCount = thread->wakeUpCount;
	thread->wakeUpCount = 0;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = wakeUpCount;
}

//37
//38
void CPS2OS::sc_SuspendThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	bool isInt = m_ee.m_State.nGPR[3].nV[0] == 0x38;

	if(id == m_currentThreadId)
	{
		//This actually works on a real PS2
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(
	    (thread->status == THREAD_ZOMBIE) ||
	    (thread->status == THREAD_SUSPENDED) ||
	    (thread->status == THREAD_SUSPENDED_WAITING) ||
	    (thread->status == THREAD_SUSPENDED_SLEEPING))
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	switch(thread->status)
	{
	case THREAD_RUNNING:
		thread->status = THREAD_SUSPENDED;
		UnlinkThread(id);
		break;
	case THREAD_WAITING:
		thread->status = THREAD_SUSPENDED_WAITING;
		break;
	case THREAD_SLEEPING:
		thread->status = THREAD_SUSPENDED_SLEEPING;
		break;
	default:
		assert(0);
		break;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);

	if(!isInt)
	{
		ThreadShakeAndBake();
	}
}

//39
//3A
void CPS2OS::sc_ResumeThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	bool isInt = m_ee.m_State.nGPR[3].nV[0] == 0x3A;

	if(id == m_currentThreadId)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto thread = m_threads[id];
	if(!thread)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(
	    (thread->status == THREAD_ZOMBIE) ||
	    (thread->status == THREAD_RUNNING) ||
	    (thread->status == THREAD_WAITING) ||
	    (thread->status == THREAD_SLEEPING))
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	switch(thread->status)
	{
	case THREAD_SUSPENDED:
		thread->status = THREAD_RUNNING;
		LinkThread(id);
		break;
	case THREAD_SUSPENDED_WAITING:
		thread->status = THREAD_WAITING;
		break;
	case THREAD_SUSPENDED_SLEEPING:
		thread->status = THREAD_SLEEPING;
		break;
	default:
		assert(0);
		break;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);

	if(!isInt)
	{
		ThreadShakeAndBake();
	}
}

//3C
void CPS2OS::sc_SetupThread()
{
	uint32 stackBase = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 stackSize = m_ee.m_State.nGPR[SC_PARAM2].nV[0];

	uint32 stackAddr = 0;
	if(stackBase == 0xFFFFFFFF)
	{
		//We need to substract 4k from RAM size because some games (Espgaluda) rely on the
		//stack and heap being a very precise size. EE kernel seems to substract that amount too.
		stackAddr = m_ramSize - (4 * 1024);
	}
	else
	{
		stackAddr = stackBase + stackSize;
	}

	uint32 argsBase = m_ee.m_State.nGPR[SC_PARAM3].nV[0];
	//Copy arguments
	{
		uint32 argsCount = static_cast<uint32>(m_currentArguments.size());

		*reinterpret_cast<uint32*>(m_ram + argsBase) = argsCount;
		uint32 argsPtrs = argsBase + 4;
		//We add 1 to argsCount because argv[argc] must be equal to 0
		uint32 argsPayload = argsPtrs + ((argsCount + 1) * 4);
		for(uint32 i = 0; i < argsCount; i++)
		{
			const auto& currentArg = m_currentArguments[i];
			*reinterpret_cast<uint32*>(m_ram + argsPtrs + (i * 4)) = argsPayload;
			uint32 argSize = static_cast<uint32>(currentArg.size()) + 1;
			memcpy(m_ram + argsPayload, currentArg.c_str(), argSize);
			argsPayload += argSize;
		}
		//Set argv[argc] to 0
		*reinterpret_cast<uint32*>(m_ram + argsPtrs + (argsCount * 4)) = 0;
	}

	uint32 threadId = -1;
	if(
	    (m_currentThreadId == 0) ||
	    (m_currentThreadId == m_idleThreadId))
	{
		//No thread has been started, spawn a new thread
		threadId = m_threads.AllocateAt(m_state->allocateThreadNextId);

		//Some games (Metropolismania 2) expect main thread's id to be 1
		assert(threadId == 1);
	}
	else
	{
		//Re-use the calling thread (needed by 007: Nightfire)
		threadId = m_currentThreadId;
		UnlinkThread(threadId);
	}
	assert(static_cast<int32>(threadId) != -1);

	//Set up the main thread
	//Priority needs to be 0 because some games rely on this
	//by calling RotateThreadReadyQueue(0) (Dynasty Warriors 2)
	auto thread = m_threads[threadId];
	thread->status = THREAD_RUNNING;
	thread->stackBase = stackAddr - stackSize;
	thread->stackSize = stackSize;
	thread->initPriority = 0;
	thread->currPriority = 0;
	thread->contextPtr = 0;

	LinkThread(threadId);
	m_currentThreadId = threadId;

	uint32 stackPtr = stackAddr - STACKRES;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = stackPtr;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//3D
void CPS2OS::sc_SetupHeap()
{
	auto thread = m_threads[m_currentThreadId];

	uint32 heapBase = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 heapSize = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	if(heapSize == 0xFFFFFFFF)
	{
		thread->heapBase = thread->stackBase;
	}
	else
	{
		thread->heapBase = heapBase + heapSize;
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = thread->heapBase;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//3E
void CPS2OS::sc_EndOfHeap()
{
	auto thread = m_threads[m_currentThreadId];

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = thread->heapBase;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//40
void CPS2OS::sc_CreateSema()
{
	auto semaParam = reinterpret_cast<SEMAPHOREPARAM*>(GetStructPtr(m_ee.m_State.nGPR[SC_PARAM0].nV0));

	uint32 id = m_semaphores.Allocate();
	if(id == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	auto sema = m_semaphores[id];
	sema->count = semaParam->initCount;
	sema->maxCount = semaParam->maxCount;
	sema->option = semaParam->option;
	sema->waitCount = 0;
	sema->waitNextId = 0;

	assert(sema->count <= sema->maxCount);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//41
void CPS2OS::sc_DeleteSema()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	//Set return value now because we might reschedule
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);

	//Release all threads waiting for this semaphore
	if(sema->waitCount != 0)
	{
		while(sema->waitCount != 0)
		{
			SemaReleaseSingleThread(id, true);
		}
		ThreadShakeAndBake();
	}

	m_semaphores.Free(id);
}

//42
//43
void CPS2OS::sc_SignalSema()
{
	bool isInt = m_ee.m_State.nGPR[3].nV[0] == 0x43;
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to signal an invalid semaphore (%d).\r\n", id);
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	m_idleEvaluator.NotifyEvent(Ee::CIdleEvaluator::EVENT_SIGNALSEMA, id);

	//TODO: Check maximum value

	//Set return value here because we might reschedule
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);

	if(sema->waitCount != 0)
	{
		SemaReleaseSingleThread(id, false);
		if(!isInt)
		{
			ThreadShakeAndBake();
		}
	}
	else
	{
		sema->count++;
	}
}

//44
void CPS2OS::sc_WaitSema()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to wait an invalid semaphore (%d).\r\n", id);
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	m_idleEvaluator.NotifyEvent(Ee::CIdleEvaluator::EVENT_WAITSEMA, id);

	if(sema->count == 0)
	{
		//Put this thread in sleep mode and reschedule...
		auto thread = m_threads[m_currentThreadId];
		assert(thread->status == THREAD_RUNNING);
		thread->status = THREAD_WAITING;

		UnlinkThread(m_currentThreadId);
		SemaLinkThread(id, m_currentThreadId);
		ThreadShakeAndBake();

		return;
	}

	if(sema->count != 0)
	{
		sema->count--;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//45
//46
void CPS2OS::sc_PollSema()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	if(sema->count == 0)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	sema->count--;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//47
//48
void CPS2OS::sc_ReferSemaStatus()
{
	FRAMEWORK_MAYBE_UNUSED bool isInt = m_ee.m_State.nGPR[3].nV[0] != 0x47;
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	auto semaParam = reinterpret_cast<SEMAPHOREPARAM*>(GetStructPtr(m_ee.m_State.nGPR[SC_PARAM1].nV0));

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	semaParam->count = sema->count;
	semaParam->maxCount = sema->maxCount;
	semaParam->waitThreads = sema->waitCount;
	semaParam->option = sema->option;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//4B
void CPS2OS::sc_GetOsdConfigParam()
{
	auto language = static_cast<OSD_LANGUAGE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_SYSTEM_LANGUAGE));
	auto widescreenOutput = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSHANDLER_WIDESCREEN);

	auto configParam = make_convertible<OSDCONFIGPARAM>(0);
	configParam.version = static_cast<uint32>(OSD_VERSION::V2_EXT);
	configParam.jpLanguage = (language == OSD_LANGUAGE::JAPANESE) ? 0 : 1;
	configParam.language = static_cast<uint32>(language);
	configParam.screenType = static_cast<uint32>(widescreenOutput ? OSD_SCREENTYPE::RATIO_16_9 : OSD_SCREENTYPE::RATIO_4_3);

	auto configParamPtr = reinterpret_cast<uint32*>(GetStructPtr(m_ee.m_State.nGPR[SC_PARAM0].nV0));
	(*configParamPtr) = configParam;
}

//63
void CPS2OS::sc_GetCop0()
{
	uint32 regId = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	assert(regId < 0x20);
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(m_ee.m_State.nCOP0[regId & 0x1F]);
}

//64
void CPS2OS::sc_FlushCache()
{
	FRAMEWORK_MAYBE_UNUSED uint32 operationType = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
}

//70
void CPS2OS::sc_GsGetIMR()
{
	uint32 result = 0;

	if(m_gs != NULL)
	{
		result = m_gs->ReadPrivRegister(CGSHandler::GS_IMR);
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(result);
}

//71
void CPS2OS::sc_GsPutIMR()
{
	uint32 imr = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	if(m_gs != NULL)
	{
		m_gs->WritePrivRegister(CGSHandler::GS_IMR + 0, imr);
		m_gs->WritePrivRegister(CGSHandler::GS_IMR + 4, 0);
	}
}

//73
void CPS2OS::sc_SetVSyncFlag()
{
	uint32 ptr1 = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 ptr2 = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	SetVsyncFlagPtrs(ptr1, ptr2);

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//74
void CPS2OS::sc_SetSyscall()
{
	uint32 number = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 address = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	if(number < 0x100)
	{
		GetCustomSyscallTable()[number] = address;
	}
	else if(number == 0x12C)
	{
		//This syscall doesn't do any bounds checking and it's possible to
		//set INTC handler for timer 3 through this system call (a particular version of libcdvd does this)
		//My guess is that the BIOS doesn't process custom INTC handlers for INT12 (timer 3) and the
		//only way to have an handler placed on that interrupt line is to do that trick.

		unsigned int line = 12;

		uint32 handlerId = m_intcHandlers.Allocate();
		if(static_cast<int32>(handlerId) == -1)
		{
			CLog::GetInstance().Warn(LOG_NAME, "Couldn't set INTC handler through SetSyscall");
			return;
		}

		auto handler = m_intcHandlers[handlerId];
		handler->address = address & 0x1FFFFFFF;
		handler->cause = line;
		handler->arg = 0;
		handler->gp = 0;

		if(!(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & (1 << line)))
		{
			m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, (1 << line));
		}

		m_intcHandlerQueue.PushFront(handlerId);
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, "Unknown syscall set (%d).\r\n", number);
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//76
void CPS2OS::sc_SifDmaStat()
{
	uint32 queueId = m_ee.m_State.nGPR[SC_PARAM0].nV0;
	uint32 queueIdx = queueId - BIOS_ID_BASE;
	if(queueIdx >= BIOS_SIFDMA_COUNT)
	{
		//Act as if it was completed
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	//If SIF dma has just been set (1000 cycle delay), return 'queued' status.
	//This is required for Okami & God Hand (Clover Studio games) which expect to see the queued status.
	int64 timerDiff = static_cast<uint64>(m_ee.m_State.nCOP0[CCOP_SCU::COUNT]) - static_cast<uint64>(m_sifDmaTimes[queueIdx]);
	if((timerDiff < 0) || (timerDiff > 1000))
	{
		//Completed
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
	}
	else
	{
		//Queued
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(1);
	}
}

//77
void CPS2OS::sc_SifSetDma()
{
	uint32 queueIdx = m_sifDmaNextIdx;
	m_sifDmaTimes[queueIdx] = m_ee.m_State.nCOP0[CCOP_SCU::COUNT];
	m_sifDmaNextIdx = (m_sifDmaNextIdx + 1) % BIOS_SIFDMA_COUNT;

	auto xfers = reinterpret_cast<const SIFDMAREG*>(GetStructPtr(m_ee.m_State.nGPR[SC_PARAM0].nV0));
	uint32 count = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	//SifSetDma seems to always send something
	if(count == 0) count = 1;

	//DMA might call an interrupt handler, set return value now
	uint32 queueId = queueIdx + BIOS_ID_BASE;
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(queueId);

	for(unsigned int i = 0; i < count; i++)
	{
		const auto& xfer = xfers[i];

		uint32 size = (xfer.size + 0x0F) / 0x10;
		assert((xfer.srcAddr & 0x0F) == 0);
		assert((xfer.dstAddr & 0x03) == 0);

		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_MADR, xfer.srcAddr);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_TADR, xfer.dstAddr);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_QWC, size);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_CHCR, CDMAC::CHCR_BIT::CHCR_STR);
	}
}

//78
void CPS2OS::sc_SifSetDChain()
{
	//Humm, set the SIF0 DMA channel in destination chain mode?

	//This syscall is invoked by the SIF dma interrupt handler (when a packet has been received)
	//To make sure every packet generates an interrupt, the SIF will hold into any packet
	//that it might have in its queue while a packet is being processed.
	//The MarkPacketProcess function will tell the SIF that the packet has been processed
	//and that it can continue sending packets over. (Needed for Guilty Gear XX Accent Core Plus)
	bool isInt = m_ee.m_State.nGPR[3].nV[1] == ~0;
	if(isInt)
	{
		m_sif.MarkPacketProcessed();
	}
}

//79
void CPS2OS::sc_SifSetReg()
{
	uint32 registerId = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 value = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	m_sif.SetRegister(registerId, value);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = 0;
}

//7A
void CPS2OS::sc_SifGetReg()
{
	uint32 registerId = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(m_sif.GetRegister(registerId));
}

//7C
void CPS2OS::sc_Deci2Call()
{
	uint32 function = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 param = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	switch(function)
	{
	case 0x01:
		//Deci2Open
		{
			uint32 id = GetNextAvailableDeci2HandlerId();

			auto handler = GetDeci2Handler(id);
			handler->valid = 1;
			handler->device = *reinterpret_cast<uint32*>(GetStructPtr(param + 0x00));
			handler->bufferAddr = *reinterpret_cast<uint32*>(GetStructPtr(param + 0x04));

			m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
		}
		break;
	case 0x03:
		//Deci2Send
		{
			uint32 id = *reinterpret_cast<uint32*>(GetStructPtr(param + 0x00));
			auto handler = GetDeci2Handler(id);

			assert(handler->valid);
			if(handler->valid)
			{
				auto buffer = reinterpret_cast<DECI2BUFFER*>(GetStructPtr(handler->bufferAddr));
				if(buffer->dataAddr != 0)
				{
					auto sendInfo = reinterpret_cast<DECI2SEND*>(GetStructPtr(buffer->dataAddr));
					assert(sendInfo->size >= 0x0C);
					if(sendInfo->size >= 0x0C)
					{
						m_iopBios.GetIoman()->Write(Iop::CIoman::FID_STDOUT, sendInfo->size - 0xC, sendInfo->data);
					}
					buffer->status0 = 0;
				}
				else
				{
					//Report an error. F1 2002 seems to send something wrong and expects some error.
					buffer->status0 = -1;
				}
			}

			m_ee.m_State.nGPR[SC_RETURN].nD0 = 1;
		}
		break;
	case 0x04:
		//Deci2Poll
		{
			uint32 id = *reinterpret_cast<uint32*>(GetStructPtr(param + 0x00));

			auto handler = GetDeci2Handler(id);
			assert(handler->valid);
			if(handler->valid)
			{
				auto buffer = reinterpret_cast<DECI2BUFFER*>(GetStructPtr(handler->bufferAddr));
				buffer->status1 = 0;
			}

			m_ee.m_State.nGPR[SC_RETURN].nD0 = 1;
		}
		break;
	case 0x10:
		//kPuts
		{
			uint32 stringAddr = *reinterpret_cast<uint32*>(GetStructPtr(param));
			uint8* string = &m_ram[stringAddr];
			m_iopBios.GetIoman()->Write(1, static_cast<uint32>(strlen(reinterpret_cast<char*>(string))), string);
		}
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown Deci2Call function (0x%08X) called. PC: 0x%08X.\r\n", function, m_ee.m_State.nPC);
		break;
	}
}

//7E
void CPS2OS::sc_MachineType()
{
	//Return 0x100 for liberx (is this ok?)
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0x100;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//7F
void CPS2OS::sc_GetMemorySize()
{
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = m_ramSize;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//////////////////////////////////////////////////
//System Call Handler
//////////////////////////////////////////////////

void CPS2OS::HandleSyscall()
{
	uint32 searchAddress = m_ee.m_State.nCOP0[CCOP_SCU::EPC];
	uint32 callInstruction = m_ee.m_pMemoryMap->GetInstruction(searchAddress);
	if(callInstruction != 0x0000000C)
	{
		//This will happen if an ADDIU R0, R0, $x instruction is encountered. Not sure if there's a use for that on the EE
		CLog::GetInstance().Warn(LOG_NAME, "System call exception occured but no SYSCALL instruction found (addr = 0x%08X, opcode = 0x%08X).\r\n",
		                         searchAddress, callInstruction);
		m_ee.m_State.nHasException = 0;
		return;
	}

	uint32 func = m_ee.m_State.nGPR[CMIPS::V1].nV[0];

	if(func == SYSCALL_CUSTOM_RESCHEDULE)
	{
		//Reschedule
		ThreadShakeAndBake();
	}
	else if(func == SYSCALL_CUSTOM_EXITINTERRUPT)
	{
		//Execute ERET
		m_ee.m_State.nCOP0[CCOP_SCU::STATUS] &= ~(CMIPS::STATUS_EXL);
		m_ee.m_State.nPC = m_ee.m_State.nGPR[CMIPS::A0].nV0;

		if(m_currentThreadId != m_idleThreadId)
		{
			auto thread = m_threads[m_currentThreadId];
			ThreadLoadContext(thread, true);
		}
		ThreadShakeAndBake();
	}
	else if((func >= Ee::CLibMc2::SYSCALL_RANGE_START) && (func < Ee::CLibMc2::SYSCALL_RANGE_END))
	{
		m_libMc2.HandleSyscall(m_ee);
	}
	else
	{
		if(func & 0x80000000)
		{
			func = 0 - func;
		}
		//Save for custom handler
		m_ee.m_State.nGPR[3].nV[0] = func;

		if(GetCustomSyscallTable()[func] == 0)
		{
#ifdef _DEBUG
			DisassembleSysCall(static_cast<uint8>(func & 0xFF));
#endif
			if(func < 0x80)
			{
				((this)->*(m_sysCall[func & 0xFF]))();
			}
		}
		else
		{
			m_ee.GenerateException(0x1FC00100);
		}
	}

	m_ee.m_State.nHasException = MIPS_EXCEPTION_NONE;
}

void CPS2OS::HandleTLBException()
{
	assert(m_ee.CanGenerateInterrupt());
	m_ee.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_EXL;
	assert(m_ee.m_State.nDelayedJumpAddr == MIPS_INVALID_PC);

	uint32 excCode = m_ee.m_State.nCOP0[CCOP_SCU::CAUSE] & CCOP_SCU::CAUSE_EXCCODE_MASK;
	switch(excCode)
	{
	case CCOP_SCU::CAUSE_EXCCODE_TLBL:
		m_ee.m_State.nPC = m_tlblExceptionHandler;
		break;
	case CCOP_SCU::CAUSE_EXCCODE_TLBS:
		m_ee.m_State.nPC = m_tlbsExceptionHandler;
		break;
	default:
		//Can't handle other types of exceptions here
		assert(false);
		break;
	}
	m_ee.m_State.nHasException = MIPS_EXCEPTION_NONE;
}

void CPS2OS::DisassembleSysCall(uint8 func)
{
#ifdef _DEBUG
	std::string description(GetSysCallDescription(func));
	if(description.length() != 0)
	{
		CLog::GetInstance().Print(LOG_NAME, "%d: %s\r\n", m_currentThreadId.Get(), description.c_str());
	}
#endif
}

std::string CPS2OS::GetSysCallDescription(uint8 function)
{
	std::string description;

	switch(function)
	{
	case 0x02:
		description = string_format("GsSetCrt(interlace = %i, mode = %i, field = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x04:
		description = string_format(SYSCALL_NAME_EXIT "();");
		break;
	case 0x06:
		description = string_format(SYSCALL_NAME_LOADEXECPS2 "(exec = 0x%08X, argc = %d, argv = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x07:
		description = string_format(SYSCALL_NAME_EXECPS2 "(pc = 0x%08X, gp = 0x%08X, argc = %d, argv = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM3].nV[0]);
		break;
	case 0x0D:
		description = string_format(SYSCALL_NAME_SETVTLBREFILLHANDLER "(cause = %d, handler = 0x%08x);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x0E:
		description = string_format(SYSCALL_NAME_SETVCOMMONHANDLER "(cause = %d, handler = 0x%08x);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x10:
		description = string_format(SYSCALL_NAME_ADDINTCHANDLER "(cause = %i, address = 0x%08X, next = 0x%08X, arg = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM3].nV[0]);
		break;
	case 0x11:
		description = string_format(SYSCALL_NAME_REMOVEINTCHANDLER "(cause = %i, id = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x12:
		description = string_format(SYSCALL_NAME_ADDDMACHANDLER "(channel = %i, address = 0x%08X, next = %i, arg = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM3].nV[0]);
		break;
	case 0x13:
		description = string_format(SYSCALL_NAME_REMOVEDMACHANDLER "(channel = %i, handler = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x14:
		description = string_format(SYSCALL_NAME_ENABLEINTC "(cause = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x15:
		description = string_format(SYSCALL_NAME_DISABLEINTC "(cause = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x16:
		description = string_format(SYSCALL_NAME_ENABLEDMAC "(channel = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x17:
		description = string_format(SYSCALL_NAME_DISABLEDMAC "(channel = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x18:
		description = string_format(SYSCALL_NAME_SETALARM "(time = %d, proc = 0x%08X, arg = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x1A:
		description = string_format(SYSCALL_NAME_IENABLEINTC "(cause = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x1B:
		description = string_format(SYSCALL_NAME_IDISABLEINTC "(cause = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x1C:
		description = string_format(SYSCALL_NAME_IENABLEDMAC "(channel = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x1D:
		description = string_format(SYSCALL_NAME_IDISABLEDMAC "(channel = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x1E:
		description = string_format(SYSCALL_NAME_ISETALARM "(time = %d, proc = 0x%08X, arg = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x1F:
		description = string_format(SYSCALL_NAME_IRELEASEALARM "(id = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x20:
		description = string_format(SYSCALL_NAME_CREATETHREAD "(thread = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x21:
		description = string_format(SYSCALL_NAME_DELETETHREAD "(id = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x22:
		description = string_format(SYSCALL_NAME_STARTTHREAD "(id = 0x%08X, a0 = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x23:
		description = string_format(SYSCALL_NAME_EXITTHREAD "();");
		break;
	case 0x24:
		description = string_format(SYSCALL_NAME_EXITDELETETHREAD "();");
		break;
	case 0x25:
		description = string_format(SYSCALL_NAME_TERMINATETHREAD "(id = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x29:
		description = string_format(SYSCALL_NAME_CHANGETHREADPRIORITY "(id = 0x%08X, priority = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x2A:
		description = string_format(SYSCALL_NAME_ICHANGETHREADPRIORITY "(id = 0x%08X, priority = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x2B:
		description = string_format(SYSCALL_NAME_ROTATETHREADREADYQUEUE "(prio = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x2D:
		description = string_format(SYSCALL_NAME_RELEASEWAITTHREAD "(id = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x2E:
		description = string_format(SYSCALL_NAME_IRELEASEWAITTHREAD "(id = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x2F:
		description = string_format(SYSCALL_NAME_GETTHREADID "();");
		break;
	case 0x30:
		description = string_format(SYSCALL_NAME_REFERTHREADSTATUS "(threadId = %d, infoPtr = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x31:
		description = string_format(SYSCALL_NAME_IREFERTHREADSTATUS "(threadId = %d, infoPtr = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x32:
		description = string_format(SYSCALL_NAME_SLEEPTHREAD "();");
		break;
	case 0x33:
		description = string_format(SYSCALL_NAME_WAKEUPTHREAD "(id = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x34:
		description = string_format(SYSCALL_NAME_IWAKEUPTHREAD "(id = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x35:
		description = string_format(SYSCALL_NAME_CANCELWAKEUPTHREAD "(id = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x36:
		description = string_format(SYSCALL_NAME_ICANCELWAKEUPTHREAD "(id = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x37:
		description = string_format(SYSCALL_NAME_SUSPENDTHREAD "(id = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x38:
		description = string_format(SYSCALL_NAME_ISUSPENDTHREAD "(id = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x39:
		description = string_format(SYSCALL_NAME_RESUMETHREAD "(id = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x3C:
		description = string_format("SetupThread(gp = 0x%08X, stack = 0x%08X, stack_size = 0x%08X, args = 0x%08X, root_func = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM2].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM3].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM4].nV[0]);
		break;
	case 0x3D:
		description = string_format("SetupHeap(heap_start = 0x%08X, heap_size = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x3E:
		description = string_format(SYSCALL_NAME_ENDOFHEAP "();");
		break;
	case 0x40:
		description = string_format(SYSCALL_NAME_CREATESEMA "(sema = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x41:
		description = string_format(SYSCALL_NAME_DELETESEMA "(semaid = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x42:
		description = string_format(SYSCALL_NAME_SIGNALSEMA "(semaid = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x43:
		description = string_format(SYSCALL_NAME_ISIGNALSEMA "(semaid = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x44:
		description = string_format(SYSCALL_NAME_WAITSEMA "(semaid = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x45:
		description = string_format(SYSCALL_NAME_POLLSEMA "(semaid = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x46:
		description = string_format(SYSCALL_NAME_IPOLLSEMA "(semaid = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x47:
		description = string_format(SYSCALL_NAME_REFERSEMASTATUS "(semaid = %i, status = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x48:
		description = string_format(SYSCALL_NAME_IREFERSEMASTATUS "(semaid = %i, status = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x4B:
		description = string_format(SYSCALL_NAME_GETOSDCONFIGPARAM "(configPtr = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x63:
		description = string_format(SYSCALL_NAME_GETCOP0 "(reg = %d);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x64:
	case 0x68:
#ifdef _DEBUG
//		description = string_format(SYSCALL_NAME_FLUSHCACHE "();");
#endif
		break;
	case 0x70:
		description = string_format(SYSCALL_NAME_GSGETIMR "();");
		break;
	case 0x71:
		description = string_format(SYSCALL_NAME_GSPUTIMR "(GS_IMR = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x73:
		description = string_format(SYSCALL_NAME_SETVSYNCFLAG "(ptr1 = 0x%08X, ptr2 = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x74:
		description = string_format(SYSCALL_NAME_SETSYSCALL "(num = 0x%02X, address = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x76:
		description = string_format(SYSCALL_NAME_SIFDMASTAT "();");
		break;
	case 0x77:
		description = string_format(SYSCALL_NAME_SIFSETDMA "(list = 0x%08X, count = %i);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x78:
		description = string_format(SYSCALL_NAME_SIFSETDCHAIN "();");
		break;
	case 0x79:
		description = string_format("SifSetReg(register = 0x%08X, value = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7A:
		description = string_format("SifGetReg(register = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x7C:
		description = string_format(SYSCALL_NAME_DECI2CALL "(func = 0x%08X, param = 0x%08X);",
		                            m_ee.m_State.nGPR[SC_PARAM0].nV[0],
		                            m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7E:
		description = string_format(SYSCALL_NAME_MACHINETYPE "();");
		break;
	case 0x7F:
		description = string_format("GetMemorySize();");
		break;
	}

	return description;
}

//////////////////////////////////////////////////
//System Call Handlers Table
//////////////////////////////////////////////////

// clang-format off
CPS2OS::SystemCallHandler CPS2OS::m_sysCall[0x80] =
{
	//0x00
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_GsSetCrt,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Exit,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_LoadExecPS2,		&CPS2OS::sc_ExecPS2,
	//0x08
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_SetVTLBRefillHandler,	&CPS2OS::sc_SetVCommonHandler,	&CPS2OS::sc_Unhandled,
	//0x10
	&CPS2OS::sc_AddIntcHandler,		&CPS2OS::sc_RemoveIntcHandler,		&CPS2OS::sc_AddDmacHandler,			&CPS2OS::sc_RemoveDmacHandler,		&CPS2OS::sc_EnableIntc,			&CPS2OS::sc_DisableIntc,			&CPS2OS::sc_EnableDmac,			&CPS2OS::sc_DisableDmac,
	//0x18
	&CPS2OS::sc_SetAlarm,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_EnableIntc,				&CPS2OS::sc_DisableIntc,			&CPS2OS::sc_EnableDmac,			&CPS2OS::sc_DisableDmac,			&CPS2OS::sc_SetAlarm,			&CPS2OS::sc_ReleaseAlarm,
	//0x20
	&CPS2OS::sc_CreateThread,		&CPS2OS::sc_DeleteThread,			&CPS2OS::sc_StartThread,			&CPS2OS::sc_ExitThread,				&CPS2OS::sc_ExitDeleteThread,	&CPS2OS::sc_TerminateThread,		&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x28
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_ChangeThreadPriority,	&CPS2OS::sc_ChangeThreadPriority,	&CPS2OS::sc_RotateThreadReadyQueue,	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_ReleaseWaitThread,		&CPS2OS::sc_ReleaseWaitThread,	&CPS2OS::sc_GetThreadId,
	//0x30
	&CPS2OS::sc_ReferThreadStatus,	&CPS2OS::sc_ReferThreadStatus,		&CPS2OS::sc_SleepThread,			&CPS2OS::sc_WakeupThread,			&CPS2OS::sc_WakeupThread,		&CPS2OS::sc_CancelWakeupThread,		&CPS2OS::sc_CancelWakeupThread,	&CPS2OS::sc_SuspendThread,
	//0x38
	&CPS2OS::sc_SuspendThread,		&CPS2OS::sc_ResumeThread,			&CPS2OS::sc_ResumeThread,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_SetupThread,		&CPS2OS::sc_SetupHeap,				&CPS2OS::sc_EndOfHeap,			&CPS2OS::sc_Unhandled,
	//0x40
	&CPS2OS::sc_CreateSema,			&CPS2OS::sc_DeleteSema,				&CPS2OS::sc_SignalSema,				&CPS2OS::sc_SignalSema,				&CPS2OS::sc_WaitSema,			&CPS2OS::sc_PollSema,				&CPS2OS::sc_PollSema,			&CPS2OS::sc_ReferSemaStatus,
	//0x48
	&CPS2OS::sc_ReferSemaStatus,	&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_GetOsdConfigParam,		&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x50
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x58
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x60
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_GetCop0,				&CPS2OS::sc_FlushCache,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x68
	&CPS2OS::sc_FlushCache,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x70
	&CPS2OS::sc_GsGetIMR,			&CPS2OS::sc_GsPutIMR,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_SetVSyncFlag,			&CPS2OS::sc_SetSyscall,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_SifDmaStat,			&CPS2OS::sc_SifSetDma,
	//0x78
	&CPS2OS::sc_SifSetDChain,		&CPS2OS::sc_SifSetReg,				&CPS2OS::sc_SifGetReg,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Deci2Call,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_MachineType,		&CPS2OS::sc_GetMemorySize,
};
// clang-format on

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Debug Stuff

#ifdef DEBUGGER_INCLUDED

BiosDebugModuleInfoArray CPS2OS::GetModulesDebugInfo() const
{
	BiosDebugModuleInfoArray result;

	if(m_elf)
	{
		auto executableRange = GetExecutableRange();

		BIOS_DEBUG_MODULE_INFO module;
		module.name = m_executableName;
		module.begin = executableRange.first;
		module.end = executableRange.second;
		module.param = m_elf.get();
		result.push_back(module);
	}

	return result;
}

enum EE_BIOS_DEBUG_OBJECT_TYPE
{
	EE_BIOS_DEBUG_OBJECT_TYPE_INTCHANDLER = BIOS_DEBUG_OBJECT_TYPE_CUSTOM_START,
	EE_BIOS_DEBUG_OBJECT_TYPE_DMACHANDLER,
	EE_BIOS_DEBUG_OBJECT_TYPE_SEMAPHORE,
};

BiosDebugObjectInfoMap CPS2OS::GetBiosObjectsDebugInfo() const
{
	static BiosDebugObjectInfoMap objectDebugInfo = [] {
		BiosDebugObjectInfoMap result;
		{
			BIOS_DEBUG_OBJECT_INFO info;
			info.name = "Semaphores";
			info.fields =
			    {
			        {"Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::IDENTIFIER},
			        {"Option", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::POSSIBLE_STR_POINTER},
			        {"Count", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			        {"Max Count", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			        {"Wait Count", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			    };
			result.emplace(std::make_pair(EE_BIOS_DEBUG_OBJECT_TYPE_SEMAPHORE, std::move(info)));
		}
		{
			BIOS_DEBUG_OBJECT_INFO info;
			info.name = "INTC Handlers";
			info.selectionAction = BIOS_DEBUG_OBJECT_ACTION::SHOW_LOCATION;
			info.fields =
			    {
			        {"Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::IDENTIFIER},
			        {"Cause", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			        {"Address", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::LOCATION | BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::TEXT_ADDRESS},
			        {"Parameter", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			        {"GP", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::DATA_ADDRESS},
			        {"Next Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			    };
			result.emplace(std::make_pair(EE_BIOS_DEBUG_OBJECT_TYPE_INTCHANDLER, std::move(info)));
		}
		{
			BIOS_DEBUG_OBJECT_INFO info;
			info.name = "DMAC Handlers";
			info.selectionAction = BIOS_DEBUG_OBJECT_ACTION::SHOW_LOCATION;
			info.fields =
			    {
			        {"Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::IDENTIFIER},
			        {"Channel", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			        {"Address", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::LOCATION | BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::TEXT_ADDRESS},
			        {"Parameter", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			        {"GP", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::DATA_ADDRESS},
			        {"Next Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			    };
			result.emplace(std::make_pair(EE_BIOS_DEBUG_OBJECT_TYPE_DMACHANDLER, std::move(info)));
		}
		{
			BIOS_DEBUG_OBJECT_INFO info;
			info.name = "Threads";
			info.selectionAction = BIOS_DEBUG_OBJECT_ACTION::SHOW_STACK_OR_LOCATION;
			info.fields =
			    {
			        {"Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::IDENTIFIER},
			        {"Priority", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32},
			        {"Location", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::LOCATION | BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::TEXT_ADDRESS},
			        {"RA", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::RETURN_ADDRESS | BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::HIDDEN},
			        {"SP", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::STACK_POINTER | BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::HIDDEN},
			        {"Entry Point", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::TEXT_ADDRESS},
			        {"State", BIOS_DEBUG_OBJECT_FIELD_TYPE::STRING},
			    };
			result.emplace(std::make_pair(BIOS_DEBUG_OBJECT_TYPE_THREAD, std::move(info)));
		}
		return result;
	}();
	return objectDebugInfo;
}

BiosDebugObjectArray CPS2OS::GetBiosObjects(uint32 typeId) const
{
	BiosDebugObjectArray result;
	switch(typeId)
	{
	case EE_BIOS_DEBUG_OBJECT_TYPE_SEMAPHORE:
		for(auto it = std::begin(m_semaphores); it != std::end(m_semaphores); it++)
		{
			auto semaphore = (*it);
			if(!semaphore) continue;
			BIOS_DEBUG_OBJECT obj;
			obj.fields = {
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, it),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, semaphore->option),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, semaphore->count),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, semaphore->maxCount),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, semaphore->waitCount),
			};
			result.push_back(obj);
		}
		break;
	case EE_BIOS_DEBUG_OBJECT_TYPE_INTCHANDLER:
		for(auto it = std::begin(m_intcHandlers); it != std::end(m_intcHandlers); it++)
		{
			auto handler = (*it);
			if(!handler) continue;
			BIOS_DEBUG_OBJECT obj;
			obj.fields = {
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, it),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->cause),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->address),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->arg),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->gp),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->nextId),
			};
			result.push_back(obj);
		}
		break;
	case EE_BIOS_DEBUG_OBJECT_TYPE_DMACHANDLER:
		for(auto it = std::begin(m_dmacHandlers); it != std::end(m_dmacHandlers); it++)
		{
			auto handler = (*it);
			if(!handler) continue;
			BIOS_DEBUG_OBJECT obj;
			obj.fields = {
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, it),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->channel),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->address),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->arg),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->gp),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, handler->nextId),
			};
			result.push_back(obj);
		}
		break;
	case BIOS_DEBUG_OBJECT_TYPE_THREAD:
		for(auto it = std::begin(m_threads); it != std::end(m_threads); it++)
		{
			auto thread = *it;
			if(!thread) continue;

			auto threadContext = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));

			uint32 pc = 0;
			uint32 ra = 0;
			uint32 sp = 0;

			if(m_currentThreadId == it)
			{
				pc = m_ee.m_State.nPC;
				ra = m_ee.m_State.nGPR[CMIPS::RA].nV0;
				sp = m_ee.m_State.nGPR[CMIPS::SP].nV0;
			}
			else
			{
				pc = thread->epc;
				ra = threadContext->gpr[CMIPS::RA].nV0;
				sp = threadContext->gpr[CMIPS::SP].nV0;
			}

			std::string stateDescription;
			switch(thread->status)
			{
			case THREAD_RUNNING:
				stateDescription = "Running";
				break;
			case THREAD_SLEEPING:
				stateDescription = "Sleeping";
				break;
			case THREAD_WAITING:
				stateDescription = string_format("Waiting (Semaphore: %d)", thread->semaWait);
				break;
			case THREAD_SUSPENDED:
				stateDescription = "Suspended";
				break;
			case THREAD_SUSPENDED_SLEEPING:
				stateDescription = "Suspended+Sleeping";
				break;
			case THREAD_SUSPENDED_WAITING:
				stateDescription = string_format("Suspended+Waiting (Semaphore: %d)", thread->semaWait);
				break;
			case THREAD_ZOMBIE:
				stateDescription = "Zombie";
				break;
			default:
				stateDescription = "Unknown";
				break;
			}

			BIOS_DEBUG_OBJECT obj;
			obj.fields = {
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, it),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, thread->currPriority),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, pc),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, ra),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, sp),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, thread->threadProc),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<std::string>, stateDescription),
			};
			result.push_back(obj);
		}
		break;
	}
	return result;
}

#endif
