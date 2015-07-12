#include <stddef.h>
#include <stdlib.h>
#include <exception>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include "PS2OS.h"
#include "StdStream.h"
#include "PtrMacro.h"
#include "../Ps2Const.h"
#include "../Utils.h"
#include "../ElfFile.h"
#include "../COP_SCU.h"
#include "../uint128.h"
#include "../Log.h"
#include "../iop/IopBios.h"
#include "DMAC.h"
#include "INTC.h"
#include "SIF.h"
#include "EEAssembler.h"
#include "PathUtils.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/FilteringNodeIterator.h"
#include "StdStreamUtils.h"

// PS2OS Memory Allocation
// Start		End				Description
// 0x80000010	0x80000014		Current Thread ID
// 0x80008000	0x8000A000		DECI2 Handlers
// 0x8000A000	0x8000C000		INTC Handlers
// 0x8000C000	0x8000E000		DMAC Handlers
// 0x8000E000	0x80010000		Semaphores
// 0x80010000	0x80010800		Custom System Call addresses (0x200 entries)
// 0x80010800	0x80011000		Alarms
// 0x80011000	0x80020000		Threads
// 0x80020000	0x80030000		Kernel Stack
// 0x80030000	0x80032000		Thread Linked List

// BIOS area
// Start		End				Description
// 0x1FC00100	0x1FC00200		Custom System Call handling code
// 0x1FC00200	0x1FC01000		Interrupt Handler
// 0x1FC01000	0x1FC02000		DMAC Handler
// 0x1FC02000	0x1FC03000		INTC Handler
// 0x1FC03000	0x1FC03100		Thread epilogue
// 0x1FC03100	0x1FC03200		Wait Thread Proc
// 0x1FC03200	0x1FC03300		Alarm Handler

#define BIOS_ADDRESS_KERNELSTACK_TOP		0x00030000
#define BIOS_ADDRESS_CURRENT_THREAD_ID		0x00000010
#define BIOS_ADDRESS_VSYNCFLAG_VALUE1PTR	0x00000014
#define BIOS_ADDRESS_VSYNCFLAG_VALUE2PTR	0x00000018
#define BIOS_ADDRESS_DMACHANDLER_BASE		0x0000C000
#define BIOS_ADDRESS_SEMAPHORE_BASE			0x0000E000
#define BIOS_ADDRESS_ALARM_BASE				0x00010800

#define BIOS_ADDRESS_BASE				0x1FC00000
#define BIOS_ADDRESS_INTERRUPTHANDLER	0x1FC00200
#define BIOS_ADDRESS_DMACHANDLER		0x1FC01000
#define BIOS_ADDRESS_THREADEPILOG		0x1FC03000
#define BIOS_ADDRESS_WAITTHREADPROC		0x1FC03100
#define BIOS_ADDRESS_ALARMHANDLER		0x1FC03200

#define INTERRUPTS_ENABLED_MASK			(CMIPS::STATUS_IE | CMIPS::STATUS_EIE)

#define BIOS_ID_BASE					1

#define PATCHESFILENAME		"patches.xml"
#define LOG_NAME			("ps2os")

#define THREAD_INIT_QUOTA			(15)

#define SYSCALL_NAME_EXIT					"osExit"
#define SYSCALL_NAME_LOADEXECPS2			"osLoadExecPS2"
#define SYSCALL_NAME_EXECPS2				"osExecPS2"
#define SYSCALL_NAME_ADDINTCHANDLER			"osAddIntcHandler"
#define SYSCALL_NAME_REMOVEINTCHANDLER		"osRemoveIntcHandler"
#define SYSCALL_NAME_ADDDMACHANDLER			"osAddDmacHandler"
#define SYSCALL_NAME_REMOVEDMACHANDLER		"osRemoveDmacHandler"
#define SYSCALL_NAME_ENABLEINTC				"osEnableIntc"
#define SYSCALL_NAME_DISABLEINTC			"osDisableIntc"
#define SYSCALL_NAME_ENABLEDMAC				"osEnableDmac"
#define SYSCALL_NAME_DISABLEDMAC			"osDisableDmac"
#define SYSCALL_NAME_SETALARM				"osSetAlarm"
#define SYSCALL_NAME_IRELEASEALARM			"osiReleaseAlarm"
#define SYSCALL_NAME_CREATETHREAD			"osCreateThread"
#define SYSCALL_NAME_DELETETHREAD			"osDeleteThread"
#define SYSCALL_NAME_STARTTHREAD			"osStartThread"
#define SYSCALL_NAME_EXITTHREAD				"osExitThread"
#define SYSCALL_NAME_EXITDELETETHREAD		"osExitDeleteThread"
#define SYSCALL_NAME_TERMINATETHREAD		"osTerminateThread"
#define SYSCALL_NAME_CHANGETHREADPRIORITY	"osChangeThreadPriority"
#define SYSCALL_NAME_ICHANGETHREADPRIORITY	"osiChangeThreadPriority"
#define SYSCALL_NAME_ROTATETHREADREADYQUEUE	"osRotateThreadReadyQueue"
#define SYSCALL_NAME_GETTHREADID			"osGetThreadId"
#define SYSCALL_NAME_REFERTHREADSTATUS		"osReferThreadStatus"
#define SYSCALL_NAME_IREFERTHREADSTATUS		"osiReferThreadStatus"
#define SYSCALL_NAME_SLEEPTHREAD			"osSleepThread"
#define SYSCALL_NAME_WAKEUPTHREAD			"osWakeupThread"
#define SYSCALL_NAME_IWAKEUPTHREAD			"osiWakeupThread"
#define SYSCALL_NAME_CANCELWAKEUPTHREAD		"osCancelWakeupThread"
#define SYSCALL_NAME_ICANCELWAKEUPTHREAD	"osiCancelWakeupThread"
#define SYSCALL_NAME_SUSPENDTHREAD			"osSuspendThread"
#define SYSCALL_NAME_RESUMETHREAD			"osResumeThread"
#define SYSCALL_NAME_ENDOFHEAP				"osEndOfHeap"
#define SYSCALL_NAME_CREATESEMA				"osCreateSema"
#define SYSCALL_NAME_DELETESEMA				"osDeleteSema"
#define SYSCALL_NAME_SIGNALSEMA				"osSignalSema"
#define SYSCALL_NAME_ISIGNALSEMA			"osiSignalSema"
#define SYSCALL_NAME_WAITSEMA				"osWaitSema"
#define SYSCALL_NAME_POLLSEMA				"osPollSema"
#define SYSCALL_NAME_REFERSEMASTATUS		"osReferSemaStatus"
#define SYSCALL_NAME_IREFERSEMASTATUS		"osiReferSemaStatus"
#define SYSCALL_NAME_FLUSHCACHE				"osFlushCache"
#define SYSCALL_NAME_SIFSTOPDMA				"osSifStopDma"
#define SYSCALL_NAME_GSGETIMR				"osGsGetIMR"
#define SYSCALL_NAME_GSPUTIMR				"osGsPutIMR"
#define SYSCALL_NAME_SETVSYNCFLAG			"osSetVSyncFlag"
#define SYSCALL_NAME_SIFDMASTAT				"osSifDmaStat"
#define SYSCALL_NAME_SIFSETDMA				"osSifSetDma"
#define SYSCALL_NAME_SIFSETDCHAIN			"osSifSetDChain"
#define SYSCALL_NAME_DECI2CALL				"osDeci2Call"
#define SYSCALL_NAME_MACHINETYPE			"osMachineType"

#ifdef DEBUGGER_INCLUDED

const CPS2OS::SYSCALL_NAME	CPS2OS::g_syscallNames[] =
{
	{	0x0004,		SYSCALL_NAME_EXIT					},
	{	0x0006,		SYSCALL_NAME_LOADEXECPS2			},
	{	0x0007,		SYSCALL_NAME_EXECPS2				},
	{	0x0010,		SYSCALL_NAME_ADDINTCHANDLER			},
	{	0x0011,		SYSCALL_NAME_REMOVEINTCHANDLER		},
	{	0x0012,		SYSCALL_NAME_ADDDMACHANDLER			},
	{	0x0013,		SYSCALL_NAME_REMOVEDMACHANDLER		},
	{	0x0014,		SYSCALL_NAME_ENABLEINTC				},
	{	0x0015,		SYSCALL_NAME_DISABLEINTC			},
	{	0x0016,		SYSCALL_NAME_ENABLEDMAC				},
	{	0x0017,		SYSCALL_NAME_DISABLEDMAC			},
	{	0x0018,		SYSCALL_NAME_SETALARM				},
	{	0x001F,		SYSCALL_NAME_IRELEASEALARM			},
	{	0x0020,		SYSCALL_NAME_CREATETHREAD			},
	{	0x0021,		SYSCALL_NAME_DELETETHREAD			},
	{	0x0022,		SYSCALL_NAME_STARTTHREAD			},
	{	0x0023,		SYSCALL_NAME_EXITTHREAD				},
	{	0x0024,		SYSCALL_NAME_EXITDELETETHREAD		},
	{	0x0025,		SYSCALL_NAME_TERMINATETHREAD		},
	{	0x0029,		SYSCALL_NAME_CHANGETHREADPRIORITY	},
	{	0x002A,		SYSCALL_NAME_ICHANGETHREADPRIORITY	},
	{	0x002B,		SYSCALL_NAME_ROTATETHREADREADYQUEUE	},
	{	0x002F,		SYSCALL_NAME_GETTHREADID			},
	{	0x0030,		SYSCALL_NAME_REFERTHREADSTATUS		},
	{	0x0031,		SYSCALL_NAME_IREFERTHREADSTATUS		},
	{	0x0032,		SYSCALL_NAME_SLEEPTHREAD			},
	{	0x0033,		SYSCALL_NAME_WAKEUPTHREAD			},
	{	0x0034,		SYSCALL_NAME_IWAKEUPTHREAD			},
	{	0x0035,		SYSCALL_NAME_CANCELWAKEUPTHREAD		},
	{	0x0036,		SYSCALL_NAME_ICANCELWAKEUPTHREAD	},
	{	0x0037,		SYSCALL_NAME_SUSPENDTHREAD			},
	{	0x0039,		SYSCALL_NAME_RESUMETHREAD			},
	{	0x003E,		SYSCALL_NAME_ENDOFHEAP				},
	{	0x0040,		SYSCALL_NAME_CREATESEMA				},
	{	0x0041,		SYSCALL_NAME_DELETESEMA				},
	{	0x0042,		SYSCALL_NAME_SIGNALSEMA				},
	{	0x0043,		SYSCALL_NAME_ISIGNALSEMA			},
	{	0x0044,		SYSCALL_NAME_WAITSEMA				},
	{	0x0045,		SYSCALL_NAME_POLLSEMA				},
	{	0x0047,		SYSCALL_NAME_REFERSEMASTATUS		},
	{	0x0048,		SYSCALL_NAME_IREFERSEMASTATUS		},
	{	0x0064,		SYSCALL_NAME_FLUSHCACHE				},
	{	0x006B,		SYSCALL_NAME_SIFSTOPDMA				},
	{	0x0070,		SYSCALL_NAME_GSGETIMR				},
	{	0x0071,		SYSCALL_NAME_GSPUTIMR				},
	{	0x0073,		SYSCALL_NAME_SETVSYNCFLAG			},
	{	0x0076,		SYSCALL_NAME_SIFDMASTAT				},
	{	0x0077,		SYSCALL_NAME_SIFSETDMA				},
	{	0x0078,		SYSCALL_NAME_SIFSETDCHAIN			},
	{	0x007C,		SYSCALL_NAME_DECI2CALL				},
	{	0x007E,		SYSCALL_NAME_MACHINETYPE			},
	{	0x0000,		NULL								}
};

#endif

namespace filesystem = boost::filesystem;

CPS2OS::CPS2OS(CMIPS& ee, uint8* ram, uint8* bios, uint8* spr, CGSHandler*& gs, CSIF& sif, CIopBios& iopBios) 
: m_ee(ee)
, m_gs(gs)
, m_elf(nullptr)
, m_ram(ram)
, m_bios(bios)
, m_spr(spr)
, m_threadSchedule(nullptr)
, m_sif(sif)
, m_iopBios(iopBios)
, m_semaphores(reinterpret_cast<SEMAPHORE*>(m_ram + BIOS_ADDRESS_SEMAPHORE_BASE), BIOS_ID_BASE, MAX_SEMAPHORE)
, m_dmacHandlers(reinterpret_cast<DMACHANDLER*>(m_ram + BIOS_ADDRESS_DMACHANDLER_BASE), BIOS_ID_BASE, MAX_DMACHANDLER)
, m_alarms(reinterpret_cast<ALARM*>(m_ram + BIOS_ADDRESS_ALARM_BASE), BIOS_ID_BASE, MAX_ALARM)
{
	Initialize();
}

CPS2OS::~CPS2OS()
{
	Release();
}

void CPS2OS::Initialize()
{
	m_elf = nullptr;

	m_threadSchedule = new CRoundRibbon(m_ram + 0x30000, 0x2000);

	m_semaWaitId = -1;
	m_semaWaitCount = 0;
	m_semaWaitCaller = 0;
	m_semaWaitThreadId = -1;

	SetVsyncFlagPtrs(0, 0);

	AssembleCustomSyscallHandler();
	AssembleInterruptHandler();
	AssembleDmacHandler();
	AssembleIntcHandler();
	AssembleThreadEpilog();
	AssembleWaitThreadProc();
	AssembleAlarmHandler();
	CreateWaitThread();

	m_ee.m_State.nPC = BIOS_ADDRESS_WAITTHREADPROC;
	m_ee.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_IE;
}

void CPS2OS::Release()
{
	UnloadExecutable();
	
	DELETEPTR(m_threadSchedule);
}

bool CPS2OS::IsIdle() const
{
	return (GetCurrentThreadId() == m_semaWaitThreadId);
}

void CPS2OS::DumpIntcHandlers()
{
	printf("INTC Handlers Information\r\n");
	printf("-------------------------\r\n");

	for(unsigned int i = 0; i < MAX_INTCHANDLER; i++)
	{
		INTCHANDLER* handler = GetIntcHandler(i + 1);
		if(handler->valid == 0) continue;

		printf("ID: %0.2i, Line: %i, Address: 0x%0.8X.\r\n", \
			i + 1,
			handler->cause,
			handler->address);
	}
}

void CPS2OS::DumpDmacHandlers()
{
	printf("DMAC Handlers Information\r\n");
	printf("-------------------------\r\n");

	for(unsigned int i = 0; i < MAX_DMACHANDLER; i++)
	{
		auto handler = m_dmacHandlers[i + 1];
		if(handler == nullptr) continue;

		printf("ID: %0.2i, Channel: %i, Address: 0x%0.8X.\r\n", \
			i + 1,
			handler->channel,
			handler->address);
	}
}

void CPS2OS::BootFromFile(const char* sPath)
{
	filesystem::path execPath(sPath);
	Framework::CStdStream stream(fopen(execPath.string().c_str(), "rb"));
	LoadELF(stream, execPath.filename().string().c_str(), ArgumentList());
}

void CPS2OS::BootFromCDROM(const ArgumentList& arguments)
{
	std::string executablePath;
	Iop::CIoman* ioman = m_iopBios.GetIoman();

	{
		uint32 handle = ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, "cdrom0:SYSTEM.CNF");
		if(static_cast<int32>(handle) < 0)
		{
			throw std::runtime_error("No 'SYSTEM.CNF' file found on the cdrom0 device.");
		}

		{
			Framework::CStream* file(ioman->GetFileStream(handle));

			auto line = Utils::GetLine(file);
			while(!file->IsEOF())
			{
				auto trimmedEnd = std::remove_if(line.begin(), line.end(), isspace);
				auto trimmedLine = std::string(line.begin(), trimmedEnd);
				std::vector<std::string> components;
				boost::split(components, trimmedLine, boost::is_any_of("="), boost::algorithm::token_compress_on);
				if(components.size() >= 2)
				{
					if(components[0] == "BOOT2")
					{
						executablePath = components[1];
					}
				}
				line = Utils::GetLine(file);
			}
		}

		ioman->Close(handle);
	}

	if(executablePath.length() == 0)
	{
		throw std::runtime_error("Error parsing 'SYSTEM.CNF' for a BOOT2 value.");
	}

	{
		uint32 handle = ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, executablePath.c_str());
		if(static_cast<int32>(handle) < 0)
		{
			throw std::runtime_error("Couldn't open executable specified in SYSTEM.CNF.");
		}

		try
		{
			const char* executableName = strchr(executablePath.c_str(), ':') + 1;
			if(executableName[0] == '/' || executableName[0] == '\\') executableName++;
			Framework::CStream* file(ioman->GetFileStream(handle));
			LoadELF(*file, executableName, arguments);
		}
		catch(...)
		{
			throw std::runtime_error("Error occured while reading ELF executable from disk.");
		}
		ioman->Close(handle);
	}
}

CELF* CPS2OS::GetELF()
{
	return m_elf;
}

const char* CPS2OS::GetExecutableName() const
{
	return m_executableName.c_str();
}

std::pair<uint32, uint32> CPS2OS::GetExecutableRange() const
{
	uint32 minAddr = 0xFFFFFFF0;
	uint32 maxAddr = 0x00000000;
	const ELFHEADER& header = m_elf->GetHeader();

	for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
	{
		ELFPROGRAMHEADER* p = m_elf->GetProgram(i);
		if(p != NULL)
		{
			//Wild Arms: Alter Code F has zero sized program headers
			if(p->nFileSize == 0) continue;
			if(!(p->nFlags & CELF::PF_X)) continue;
			uint32 end = p->nVAddress + p->nFileSize;
			if(end >= PS2::EE_RAM_SIZE) continue;
			minAddr = std::min<uint32>(minAddr, p->nVAddress);
			maxAddr = std::max<uint32>(maxAddr, end);
		}
	}

	return std::pair<uint32, uint32>(minAddr, maxAddr);
}

void CPS2OS::LoadELF(Framework::CStream& stream, const char* sExecName, const ArgumentList& arguments)
{
	CELF* elf(new CElfFile(stream));

	const auto& header = elf->GetHeader();

	//Check for MIPS CPU
	if(header.nCPU != CELF::EM_MIPS)
	{
		DELETEPTR(elf);
		throw std::runtime_error("Invalid target CPU. Must be MIPS.");
	}

	if(header.nType != CELF::ET_EXEC)
	{
		DELETEPTR(elf);
		throw std::runtime_error("Not an executable ELF file.");
	}
	
	UnloadExecutable();

	m_elf = elf;

	m_executableName = sExecName;
	m_currentArguments = arguments;

	LoadExecutableInternal();
	ApplyPatches();

	OnExecutableChange();

	CLog::GetInstance().Print(LOG_NAME, "Loaded '%s' executable file.\r\n", sExecName);
}

void CPS2OS::LoadExecutableInternal()
{
	//Copy program in main RAM
	const ELFHEADER& header = m_elf->GetHeader();
	for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
	{
		auto p = m_elf->GetProgram(i);
		if(p != nullptr)
		{
			if(p->nVAddress >= PS2::EE_RAM_SIZE)
			{
				assert(false);
				continue;
			}
			memcpy(m_ram + p->nVAddress, m_elf->GetContent() + p->nOffset, p->nFileSize);
		}
	}

	m_ee.m_State.nPC = header.nEntryPoint;
	
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
			uint32 addiu	= *reinterpret_cast<uint32*>(m_ram + address - 4);
			uint32 jr		= *reinterpret_cast<uint32*>(m_ram + address + 4);
			if(
				(jr == 0x03E00008) && 
				(addiu & 0xFFFF0000) == 0x24030000
				)
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
					sprintf(syscallName, "syscall_%0.4X", syscallId);
				}
				m_ee.m_Functions.InsertTag(address - 4, syscallName);
			}
		}
	}

#endif
}

void CPS2OS::UnloadExecutable()
{
	if(m_elf == NULL) return;

	OnExecutableUnloading();

	DELETEPTR(m_elf);
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
		CElfFile executable(*fileStream);
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
	auto patchesPath = Framework::PathUtils::GetAppResourcesPath() / PATCHESFILENAME;
	
	std::unique_ptr<Framework::Xml::CNode> document;
	try
	{
		Framework::CStdStream patchesStream(Framework::CreateInputStdStream(patchesPath.native()));
		document = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(patchesStream));
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
				
				const char* addressString	= patch->GetAttribute("Address");
				const char* valueString		= patch->GetAttribute("Value");

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

	const uint32 stackFrameSize = 0x230;

	//Epilogue (allocate stackFrameSize bytes)
	assembler.LI(CMIPS::K0, BIOS_ADDRESS_KERNELSTACK_TOP);
	assembler.ADDIU(CMIPS::K0, CMIPS::K0, 0x10000 - stackFrameSize);
	
	//Save context
	for(unsigned int i = 0; i < 32; i++)
	{
		assembler.SQ(i, (i * 0x10), CMIPS::K0);
	}

	assembler.MFLO(CMIPS::V0);
	assembler.MFLO1(CMIPS::V1);
	assembler.SD(CMIPS::V0, 0x0200, CMIPS::K0);
	assembler.SD(CMIPS::V1, 0x0208, CMIPS::K0);

	assembler.MFHI(CMIPS::V0);
	assembler.MFHI1(CMIPS::V1);
	assembler.SD(CMIPS::V0, 0x0210, CMIPS::K0);
	assembler.SD(CMIPS::V1, 0x0218, CMIPS::K0);

	//Save EPC
	assembler.MFC0(CMIPS::T0, CCOP_SCU::EPC);
	assembler.SW(CMIPS::T0, 0x0220, CMIPS::K0);

	//Set SP
	assembler.ADDU(CMIPS::SP, CMIPS::K0, CMIPS::R0);

	//Get INTC status
	assembler.LUI(CMIPS::T0, 0x1000);
	assembler.ORI(CMIPS::T0, CMIPS::T0, 0xF000);
	assembler.LW(CMIPS::S0, 0x0000, CMIPS::T0);

	//Get INTC mask
	assembler.LUI(CMIPS::T1, 0x1000);
	assembler.ORI(CMIPS::T1, CMIPS::T1, 0xF010);
	assembler.LW(CMIPS::S1, 0x0000, CMIPS::T1);

	//Get cause
	assembler.AND(CMIPS::S0, CMIPS::S0, CMIPS::S1);

	//Clear cause
	//assembler.SW(CMIPS::S0, 0x0000, CMIPS::T0);
	assembler.NOP();

	static const auto generateIntHandler = 
		[](CMIPSAssembler& assembler, uint32 line)
		{
			auto skipIntHandlerLabel = assembler.CreateLabel();

			//Check cause
			assembler.ANDI(CMIPS::T0, CMIPS::S0, (1 << line));
			assembler.BEQ(CMIPS::R0, CMIPS::T0, skipIntHandlerLabel);
			assembler.NOP();

			//Process handlers
			assembler.LUI(CMIPS::T0, 0x1FC0);
			assembler.ORI(CMIPS::T0, CMIPS::T0, 0x2000);
			assembler.ADDIU(CMIPS::A0, CMIPS::R0, line);
			assembler.JALR(CMIPS::T0);
			assembler.NOP();

			assembler.MarkLabel(skipIntHandlerLabel);
		};

	generateIntHandler(assembler, CINTC::INTC_LINE_GS);
	
	{
		auto skipIntHandlerLabel = assembler.CreateLabel();

		//Check if INT1 (DMAC)
		assembler.ANDI(CMIPS::T0, CMIPS::S0, (1 << CINTC::INTC_LINE_DMAC));
		assembler.BEQ(CMIPS::R0, CMIPS::T0, skipIntHandlerLabel);
		assembler.NOP();

		//Go to DMAC interrupt handler
		assembler.JAL(BIOS_ADDRESS_DMACHANDLER);
		assembler.NOP();

		assembler.MarkLabel(skipIntHandlerLabel);
	}

	generateIntHandler(assembler, CINTC::INTC_LINE_VBLANK_START);
	generateIntHandler(assembler, CINTC::INTC_LINE_VBLANK_END);
	generateIntHandler(assembler, CINTC::INTC_LINE_VIF1);
	generateIntHandler(assembler, CINTC::INTC_LINE_TIMER0);
	generateIntHandler(assembler, CINTC::INTC_LINE_TIMER1);
	generateIntHandler(assembler, CINTC::INTC_LINE_TIMER2);
	generateIntHandler(assembler, CINTC::INTC_LINE_TIMER3);

	assembler.JAL(BIOS_ADDRESS_ALARMHANDLER);
	assembler.NOP();

	//Make sure interrupts are enabled (This is needed by some games that play
	//with the status register in interrupt handlers and is done by the EE BIOS)
	assembler.MFC0(CMIPS::T0, CCOP_SCU::STATUS);
	assembler.ORI(CMIPS::T0, CMIPS::T0, CMIPS::STATUS_IE);
	assembler.MTC0(CMIPS::T0, CCOP_SCU::STATUS);

	//Move back SP into K0 before restoring state
	assembler.ADDIU(CMIPS::K0, CMIPS::SP, CMIPS::R0);

	//Restore EPC
	assembler.LW(CMIPS::T0, 0x0220, CMIPS::K0);
	assembler.MTC0(CMIPS::T0, CCOP_SCU::EPC);

	//Restore Context
	assembler.LD(CMIPS::V0, 0x0210, CMIPS::K0);
	assembler.LD(CMIPS::V1, 0x0218, CMIPS::K0);
	assembler.MTHI(CMIPS::V0);
	assembler.MTHI1(CMIPS::V1);

	assembler.LD(CMIPS::V0, 0x0200, CMIPS::K0);
	assembler.LD(CMIPS::V1, 0x0208, CMIPS::K0);
	assembler.MTLO(CMIPS::V0);
	assembler.MTLO1(CMIPS::V1);

	for(unsigned int i = 0; i < 32; i++)
	{
		assembler.LQ(i, (i * 0x10), CMIPS::K0);
	}

	//Prologue
	assembler.ERET();
}

void CPS2OS::AssembleDmacHandler()
{
	CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_bios[BIOS_ADDRESS_DMACHANDLER - BIOS_ADDRESS_BASE]));

	auto testHandlerLabel = assembler.CreateLabel();
	auto testChannelLabel = assembler.CreateLabel();
	auto skipHandlerLabel = assembler.CreateLabel();
	auto skipChannelLabel = assembler.CreateLabel();

	//Prologue
	//S0 -> Channel Counter
	//S1 -> DMA Interrupt Status
	//S2 -> Handler Counter

	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE0);
	assembler.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.SD(CMIPS::S1, 0x0010, CMIPS::SP);
	assembler.SD(CMIPS::S2, 0x0018, CMIPS::SP);

	//Clear INTC cause
	assembler.LI(CMIPS::T1, CINTC::INTC_STAT);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, (1 << CINTC::INTC_LINE_DMAC));
	assembler.SW(CMIPS::T0, 0x0000, CMIPS::T1);

	//Load the DMA interrupt status
	assembler.LI(CMIPS::T0, CDMAC::D_STAT);
	assembler.LW(CMIPS::T0, 0x0000, CMIPS::T0);

	assembler.SRL(CMIPS::T1, CMIPS::T0, 16);
	assembler.AND(CMIPS::S1, CMIPS::T0, CMIPS::T1);

	//Initialize channel counter
	assembler.ADDIU(CMIPS::S0, CMIPS::R0, 0x0009);

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
	assembler.ADDU(CMIPS::S2, CMIPS::R0, CMIPS::R0);

	assembler.MarkLabel(testHandlerLabel);

	//Get the address to the current DMACHANDLER structure
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(DMACHANDLER));
	assembler.MULTU(CMIPS::T0, CMIPS::S2, CMIPS::T0);
	assembler.LI(CMIPS::T1, BIOS_ADDRESS_DMACHANDLER_BASE);
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Check validity
	assembler.LW(CMIPS::T1, offsetof(DMACHANDLER, isValid), CMIPS::T0);
	assembler.BEQ(CMIPS::T1, CMIPS::R0, skipHandlerLabel);
	assembler.NOP();

	//Check if the channel is good one
	assembler.LW(CMIPS::T1, offsetof(DMACHANDLER, channel), CMIPS::T0);
	assembler.BNE(CMIPS::S0, CMIPS::T1, skipHandlerLabel);
	assembler.NOP();

	//Load the necessary stuff
	assembler.LW(CMIPS::T1, offsetof(DMACHANDLER, address), CMIPS::T0);
	assembler.ADDU(CMIPS::A0, CMIPS::S0, CMIPS::R0);
	assembler.LW(CMIPS::A1, offsetof(DMACHANDLER, arg), CMIPS::T0);
	assembler.LW(CMIPS::GP, offsetof(DMACHANDLER, gp), CMIPS::T0);
	
	//Jump
	assembler.JALR(CMIPS::T1);
	assembler.NOP();

	assembler.MarkLabel(skipHandlerLabel);

	//Increment handler counter and test
	assembler.ADDIU(CMIPS::S2, CMIPS::S2, 0x0001);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, MAX_DMACHANDLER - 1);
	assembler.BNE(CMIPS::S2, CMIPS::T0, testHandlerLabel);
	assembler.NOP();

	assembler.MarkLabel(skipChannelLabel);

	//Decrement channel counter and test
	assembler.ADDIU(CMIPS::S0, CMIPS::S0, 0xFFFF);
	assembler.BGEZ(CMIPS::S0, testChannelLabel);
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
	CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_bios[0x2000]));

	CMIPSAssembler::LABEL checkHandlerLabel = assembler.CreateLabel();
	CMIPSAssembler::LABEL moveToNextHandler = assembler.CreateLabel();

	//Prologue
	//S0 -> Handler Counter

	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFE0);
	assembler.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.SD(CMIPS::S0, 0x0008, CMIPS::SP);
	assembler.SD(CMIPS::S1, 0x0010, CMIPS::SP);

	//Clear INTC cause
	assembler.LUI(CMIPS::T1, 0x1000);
	assembler.ORI(CMIPS::T1, CMIPS::T1, 0xF000);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, 0x0001);
	assembler.SLLV(CMIPS::T0, CMIPS::T0, CMIPS::A0);
	assembler.SW(CMIPS::T0, 0x0000, CMIPS::T1);

	//Initialize INTC handler loop
	assembler.ADDU(CMIPS::S0, CMIPS::R0, CMIPS::R0);
	assembler.ADDU(CMIPS::S1, CMIPS::A0, CMIPS::R0);

	assembler.MarkLabel(checkHandlerLabel);

	//Get the address to the current INTCHANDLER structure
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(INTCHANDLER));
	assembler.MULTU(CMIPS::T0, CMIPS::S0, CMIPS::T0);
	assembler.LUI(CMIPS::T1, 0x8000);
	assembler.ORI(CMIPS::T1, CMIPS::T1, 0xA000);
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Check validity
	assembler.LW(CMIPS::T1, 0x0000, CMIPS::T0);
	assembler.BEQ(CMIPS::T1, CMIPS::R0, moveToNextHandler);
	assembler.NOP();

	//Check if the cause is good one
	assembler.LW(CMIPS::T1, 0x0004, CMIPS::T0);
	assembler.BNE(CMIPS::S1, CMIPS::T1, moveToNextHandler);
	assembler.NOP();

	//Load the necessary stuff
	assembler.LW(CMIPS::T1, 0x0008, CMIPS::T0);
	assembler.ADDU(CMIPS::A0, CMIPS::S1, CMIPS::R0);
	assembler.LW(CMIPS::A1, 0x000C, CMIPS::T0);
	assembler.LW(CMIPS::GP, 0x0010, CMIPS::T0);
	
	//Jump
	assembler.JALR(CMIPS::T1);
	assembler.NOP();

	assembler.MarkLabel(moveToNextHandler);

	//Increment handler counter and test
	assembler.ADDIU(CMIPS::S0, CMIPS::S0, 0x0001);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, MAX_INTCHANDLER - 1);
	assembler.BNE(CMIPS::S0, CMIPS::T0, checkHandlerLabel);
	assembler.NOP();

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

void CPS2OS::AssembleWaitThreadProc()
{
	CMIPSAssembler assembler((uint32*)&m_bios[BIOS_ADDRESS_WAITTHREADPROC - BIOS_ADDRESS_BASE]);

	assembler.ADDIU(CMIPS::V1, CMIPS::R0, 0x666);
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

	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFF0);
	assembler.SD(CMIPS::RA, 0x0000, CMIPS::SP);
	assembler.SD(CMIPS::S0, 0x0008, CMIPS::SP);

	//Initialize handler loop
	assembler.ADDU(CMIPS::S0, CMIPS::R0, CMIPS::R0);

	assembler.MarkLabel(checkHandlerLabel);

	//Get the address to the current INTCHANDLER structure
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, sizeof(ALARM));
	assembler.MULTU(CMIPS::T0, CMIPS::S0, CMIPS::T0);
	assembler.LI(CMIPS::T1, BIOS_ADDRESS_ALARM_BASE);
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Check validity
	assembler.LW(CMIPS::T1, offsetof(ALARM, isValid), CMIPS::T0);
	assembler.BEQ(CMIPS::T1, CMIPS::R0, moveToNextHandler);
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
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0x10);
	assembler.JR(CMIPS::RA);
	assembler.NOP();
}

uint32* CPS2OS::GetCustomSyscallTable()
{
	return (uint32*)&m_ram[0x00010000];
}

uint32 CPS2OS::GetCurrentThreadId() const
{
	return *reinterpret_cast<uint32*>(m_ram + BIOS_ADDRESS_CURRENT_THREAD_ID);
}

void CPS2OS::SetCurrentThreadId(uint32 threadId)
{
	*reinterpret_cast<uint32*>(m_ram + BIOS_ADDRESS_CURRENT_THREAD_ID) = threadId;
}

uint32 CPS2OS::GetNextAvailableThreadId()
{
	for(uint32 i = 0; i < MAX_THREAD; i++)
	{
		THREAD* thread = GetThread(i);
		if(thread->valid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::THREAD* CPS2OS::GetThread(uint32 id) const
{
	return &((THREAD*)&m_ram[0x00011000])[id];
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

	//First of all, revoke the current's thread right to execute itself
	{
		unsigned int id = GetCurrentThreadId();
		if(id != 0)
		{
			THREAD* thread = GetThread(id);
			thread->quota--;
		}
	}

	//Check if all quotas expired
	if(ThreadHasAllQuotasExpired())
	{
		CRoundRibbon::ITERATOR threadIterator(m_threadSchedule);

		//If so, regive a quota to everyone
		for(threadIterator = m_threadSchedule->Begin(); !threadIterator.IsEnd(); threadIterator++)
		{
			unsigned int id = threadIterator.GetValue();
			THREAD* thread = GetThread(id);

			thread->quota = THREAD_INIT_QUOTA;
		}
	}

	//Select thread to execute
	{
		unsigned int id = 0;
		THREAD* thread = nullptr;
		CRoundRibbon::ITERATOR threadIterator(m_threadSchedule);

		//Next, find the next suitable thread to execute
		for(threadIterator = m_threadSchedule->Begin(); !threadIterator.IsEnd(); threadIterator++)
		{
			id = threadIterator.GetValue();
			thread = GetThread(id);

			if(thread->status != THREAD_RUNNING) continue;
			//if(thread->quota == 0) continue;
			break;
		}

		if(threadIterator.IsEnd())
		{
			//Deadlock or something here
			//printf("%s: Warning, no thread to execute.\r\n", LOG_NAME);
			id = 0;
		}
		else
		{
			//Remove and readd the thread into the queue
			m_threadSchedule->Remove(thread->scheduleID);
			thread->scheduleID = m_threadSchedule->Insert(id, thread->currPriority);
		}

		ThreadSwitchContext(id);
	}
}

bool CPS2OS::ThreadHasAllQuotasExpired()
{
	CRoundRibbon::ITERATOR threadIterator(m_threadSchedule);

	for(threadIterator = m_threadSchedule->Begin(); !threadIterator.IsEnd(); threadIterator++)
	{
		unsigned int id = threadIterator.GetValue();
		THREAD* thread = GetThread(id);

		if(thread->status != THREAD_RUNNING) continue;
		if(thread->quota == 0) continue;

		return false;
	}

	return true;
}

void CPS2OS::ThreadSwitchContext(unsigned int id)
{
	if(id == GetCurrentThreadId()) return;

	//Save the context of the current thread
	{
		auto thread = GetThread(GetCurrentThreadId());
		thread->contextPtr = m_ee.m_State.nGPR[CMIPS::SP].nV0 - STACKRES;

		auto context = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));

		//Save the context
		for(uint32 i = 0; i < 0x20; i++)
		{
			if(i == CMIPS::R0) continue;
			if(i == CMIPS::K0) continue;
			if(i == CMIPS::K1) continue;
			context->gpr[i] = m_ee.m_State.nGPR[i];
		}
		context->lo.nV[0] = m_ee.m_State.nLO[0];	context->lo.nV[1] = m_ee.m_State.nLO[1];
		context->hi.nV[0] = m_ee.m_State.nHI[0];	context->hi.nV[1] = m_ee.m_State.nHI[1];
		context->lo.nV[2] = m_ee.m_State.nLO1[0];	context->lo.nV[3] = m_ee.m_State.nLO1[1];
		context->hi.nV[2] = m_ee.m_State.nHI1[0];	context->hi.nV[3] = m_ee.m_State.nHI1[1];
		context->sa = m_ee.m_State.nSA;
		for(uint32 i = 0; i < THREADCONTEXT::COP1_REG_COUNT; i++)
		{
			context->cop1[i] = m_ee.m_State.nCOP1[i];
		}
		for(uint32 i = 0; i < 4; i++)
		{
			context->gpr[CMIPS::K0].nV[i] = m_ee.m_State.nCOP1[i + THREADCONTEXT::COP1_REG_COUNT];
		}
		context->cop1a = m_ee.m_State.nCOP1A;
		context->fcsr = m_ee.m_State.nFCSR;

		thread->epc = m_ee.m_State.nPC;
	}

	SetCurrentThreadId(id);

	//Load the new context
	{
		auto thread = GetThread(GetCurrentThreadId());
		m_ee.m_State.nPC = thread->epc;

		//Thread 0 doesn't have a context
		if(id != 0)
		{
			assert(thread->contextPtr != 0);
			auto context = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));

			for(uint32 i = 0; i < 0x20; i++)
			{
				if(i == CMIPS::R0) continue;
				if(i == CMIPS::K0) continue;
				if(i == CMIPS::K1) continue;
				m_ee.m_State.nGPR[i] = context->gpr[i];
			}
			m_ee.m_State.nLO[0] = context->lo.nV[0];	m_ee.m_State.nLO[1] = context->lo.nV[1];
			m_ee.m_State.nHI[0] = context->hi.nV[0];	m_ee.m_State.nHI[1] = context->hi.nV[1];
			m_ee.m_State.nLO1[0] = context->lo.nV[2];	m_ee.m_State.nLO1[1] = context->lo.nV[3];
			m_ee.m_State.nHI1[0] = context->hi.nV[2];	m_ee.m_State.nHI1[1] = context->hi.nV[3];
			m_ee.m_State.nSA = context->sa;
			for(uint32 i = 0; i < THREADCONTEXT::COP1_REG_COUNT; i++)
			{
				m_ee.m_State.nCOP1[i] = context->cop1[i];
			}
			for(uint32 i = 0; i < 4; i++)
			{
				m_ee.m_State.nCOP1[i + THREADCONTEXT::COP1_REG_COUNT] = context->gpr[CMIPS::K0].nV[i];
			}
			m_ee.m_State.nCOP1A = context->cop1a;
			m_ee.m_State.nFCSR = context->fcsr;
		}
	}

	CLog::GetInstance().Print(LOG_NAME, "New thread elected (id = %i).\r\n", id);
}

void CPS2OS::CreateWaitThread()
{
	THREAD* thread = GetThread(0);
	thread->valid		= 1;
	thread->epc			= BIOS_ADDRESS_WAITTHREADPROC;
	thread->status		= THREAD_ZOMBIE;
}

std::pair<uint32, uint32> CPS2OS::GetVsyncFlagPtrs() const
{
	uint32 value1Ptr = *reinterpret_cast<uint32*>(m_ram + BIOS_ADDRESS_VSYNCFLAG_VALUE1PTR);
	uint32 value2Ptr = *reinterpret_cast<uint32*>(m_ram + BIOS_ADDRESS_VSYNCFLAG_VALUE2PTR);
	return std::make_pair(value1Ptr, value2Ptr);
}

void CPS2OS::SetVsyncFlagPtrs(uint32 value1Ptr, uint32 value2Ptr)
{
	*reinterpret_cast<uint32*>(m_ram + BIOS_ADDRESS_VSYNCFLAG_VALUE1PTR) = value1Ptr;
	*reinterpret_cast<uint32*>(m_ram + BIOS_ADDRESS_VSYNCFLAG_VALUE2PTR) = value2Ptr;
}

uint8* CPS2OS::GetStructPtr(uint32 address) const
{
	uint8* memory = nullptr;
	if(address >= 0x70000000)
	{
		address &= (PS2::EE_SPR_SIZE - 1);
		memory = m_spr;
	}
	else
	{
		address &= (PS2::EE_RAM_SIZE - 1);
		memory = m_ram;
	}
	return memory + address;
}

uint32 CPS2OS::GetNextAvailableIntcHandlerId()
{
	for(uint32 i = 1; i < MAX_INTCHANDLER; i++)
	{
		INTCHANDLER* handler = GetIntcHandler(i);
		if(handler->valid != 1)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

CPS2OS::INTCHANDLER* CPS2OS::GetIntcHandler(uint32 id)
{
	id--;
	return &((INTCHANDLER*)&m_ram[0x0000A000])[id];
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

void CPS2OS::HandleInterrupt()
{
	//Check if interrupts are enabled here because EIE bit isn't checked by CMIPS
	if((m_ee.m_State.nCOP0[CCOP_SCU::STATUS] & INTERRUPTS_ENABLED_MASK) != INTERRUPTS_ENABLED_MASK)
	{
		return;
	}

	m_semaWaitCount = 0;
	m_ee.GenerateInterrupt(0x1FC00200);
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
		*reinterpret_cast<uint32*>(m_ram + vsyncFlagPtrs.first) = 1;
		changed = true;
	}
	if(vsyncFlagPtrs.second != 0)
	{
		*reinterpret_cast<uint64*>(m_ram + vsyncFlagPtrs.second) = m_gs->ReadPrivRegister(CGSHandler::GS_CSR);
		changed = true;
	}

	SetVsyncFlagPtrs(0, 0);
	return changed;
}

uint32 CPS2OS::TranslateAddress(CMIPS*, uint32 vaddrLo)
{
	if(vaddrLo >= 0x70000000 && vaddrLo <= 0x70003FFF)
	{
		return (vaddrLo - 0x6E000000);
	}
	if(vaddrLo >= 0x30100000 && vaddrLo <= 0x31FFFFFF)
	{
		return (vaddrLo - 0x30000000);
	}
	return vaddrLo & 0x1FFFFFFF;
}

//////////////////////////////////////////////////
//System Calls
//////////////////////////////////////////////////

void CPS2OS::sc_Unhandled()
{
	CLog::GetInstance().Print(LOG_NAME, "Unknown system call (0x%X) called from 0x%0.8X.\r\n", m_ee.m_State.nGPR[3].nV[0], m_ee.m_State.nPC);
}

//02
void CPS2OS::sc_GsSetCrt()
{
	bool isInterlaced			= (m_ee.m_State.nGPR[SC_PARAM0].nV[0] != 0);
	unsigned int mode			= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	bool isFrameMode			= (m_ee.m_State.nGPR[SC_PARAM2].nV[0] != 0);

	if(m_gs != NULL)
	{
		m_gs->SetCrt(isInterlaced, mode, isFrameMode);
	}
}


//04
void CPS2OS::sc_Exit()
{
	OnRequestExit();
}

//06
void CPS2OS::sc_LoadExecPS2()
{
	uint32 fileNamePtr	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 argCount		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 argValuesPtr	= m_ee.m_State.nGPR[SC_PARAM2].nV[0];

	ArgumentList arguments;
	for(uint32 i = 0; i < argCount; i++)
	{
		uint32 argValuePtr = *reinterpret_cast<uint32*>(m_ram + argValuesPtr + i * 4);
		arguments.push_back(reinterpret_cast<const char*>(m_ram + argValuePtr));
	}

	std::string fileName = reinterpret_cast<const char*>(m_ram + fileNamePtr);
	OnRequestLoadExecutable(fileName.c_str(), arguments);
}

//07
void CPS2OS::sc_ExecPS2()
{
	uint32 pc			= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 gp			= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 argCount		= m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	uint32 argValuesPtr	= m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	m_ee.m_State.nPC = pc;
	m_ee.m_State.nGPR[CMIPS::GP].nD0 = static_cast<int32>(gp);
	m_ee.m_State.nGPR[CMIPS::A0].nD0 = static_cast<int32>(argCount);
	m_ee.m_State.nGPR[CMIPS::A1].nD0 = static_cast<int32>(argValuesPtr);
}

//10
void CPS2OS::sc_AddIntcHandler()
{
	uint32 cause	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 address	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 next		= m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	uint32 arg		= m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	/*
	if(next != 0)
	{
		assert(0);
	}
	*/

	uint32 id = GetNextAvailableIntcHandlerId();
	if(id == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	INTCHANDLER* handler = GetIntcHandler(id);
	handler->valid		= 1;
	handler->address	= address;
	handler->cause		= cause;
	handler->arg		= arg;
	handler->gp			= m_ee.m_State.nGPR[CMIPS::GP].nV[0];

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = id;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//11
void CPS2OS::sc_RemoveIntcHandler()
{
	uint32 cause	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 id		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	INTCHANDLER* handler = GetIntcHandler(id);
	if(handler->valid != 1)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	handler->valid = 0;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//12
void CPS2OS::sc_AddDmacHandler()
{
	uint32 channel	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 address	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 next		= m_ee.m_State.nGPR[SC_PARAM2].nV[0];
	uint32 arg		= m_ee.m_State.nGPR[SC_PARAM3].nV[0];

	//The Next parameter indicates at which moment we'd want our DMAC handler to be called.
	//-1 -> At the end
	//0  -> At the start
	//n  -> After handler 'n'

	if(next != 0)
	{
		assert(0);
	}

	uint32 id = m_dmacHandlers.Allocate();
	if(id == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	auto handler = m_dmacHandlers[id];
	handler->address	= address;
	handler->channel	= channel;
	handler->arg		= arg;
	handler->gp			= m_ee.m_State.nGPR[CMIPS::GP].nV[0];

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//13
void CPS2OS::sc_RemoveDmacHandler()
{
	uint32 channel	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 id		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	auto handler = m_dmacHandlers[id];
	if(handler == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	m_dmacHandlers.Free(id);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = 0;
}

//14
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
void CPS2OS::sc_EnableDmac()
{
	uint32 channel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 registerId = 0x10000 << channel;

	if(!(m_ee.m_pMemoryMap->GetWord(CDMAC::D_STAT) & registerId))
	{
		m_ee.m_pMemoryMap->SetWord(CDMAC::D_STAT, registerId);
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
	uint32 channel = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 registerId = 0x10000 << channel;

	if(m_ee.m_pMemoryMap->GetWord(CDMAC::D_STAT) & registerId)
	{
		m_ee.m_pMemoryMap->SetWord(CDMAC::D_STAT, registerId);
		m_ee.m_State.nGPR[SC_RETURN].nD0 = 1;
	}
	else
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = 0;
	}
}

//18
void CPS2OS::sc_SetAlarm()
{
	uint32 delay			= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 callback			= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 callbackParam	= m_ee.m_State.nGPR[SC_PARAM2].nV[0];

	auto alarmId = m_alarms.Allocate();
	assert(alarmId != -1);
	if(alarmId == -1)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	auto alarm = m_alarms[alarmId];
	alarm->delay			= delay;
	alarm->callback			= callback;
	alarm->callbackParam	= callbackParam;
	alarm->gp				= m_ee.m_State.nGPR[CMIPS::GP].nV0;

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
}

//20
void CPS2OS::sc_CreateThread()
{
	auto threadParam = reinterpret_cast<THREADPARAM*>(GetStructPtr(m_ee.m_State.nGPR[SC_PARAM0].nV0));

	uint32 id = GetNextAvailableThreadId();
	if(id == 0xFFFFFFFF)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = id;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
		return;
	}

	auto parentThread = GetThread(GetCurrentThreadId());
	uint32 heapBase = parentThread->heapBase;

	assert(threadParam->initPriority < 128);

	uint32 stackAddr = threadParam->stackBase + threadParam->stackSize;

	auto thread = GetThread(id);
	thread->valid			= 1;
	thread->status			= THREAD_ZOMBIE;
	thread->stackBase		= threadParam->stackBase;
	thread->epc				= threadParam->threadProc;
	thread->threadProc		= threadParam->threadProc;
	thread->initPriority	= threadParam->initPriority;
	thread->currPriority	= threadParam->initPriority;
	thread->heapBase		= heapBase;
	thread->wakeUpCount		= 0;
	thread->quota			= THREAD_INIT_QUOTA;
	thread->scheduleID		= m_threadSchedule->Insert(id, threadParam->initPriority);
	thread->stackSize		= threadParam->stackSize;
	thread->contextPtr		= stackAddr - STACKRES;

	auto context = reinterpret_cast<THREADCONTEXT*>(&m_ram[thread->contextPtr]);
	memset(context, 0, sizeof(THREADCONTEXT));

	context->gpr[CMIPS::SP].nV0 = stackAddr;
	context->gpr[CMIPS::FP].nV0 = stackAddr;
	context->gpr[CMIPS::GP].nV0 = threadParam->gp;
	context->gpr[CMIPS::RA].nV0 = BIOS_ADDRESS_THREADEPILOG;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = id;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//21
void CPS2OS::sc_DeleteThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	if(id == GetCurrentThreadId())
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(id >= MAX_THREAD)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto thread = GetThread(id);
	if(!thread || !thread->valid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(thread->status != THREAD_ZOMBIE)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	m_threadSchedule->Remove(thread->scheduleID);

	thread->valid = 0;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
}

//22
void CPS2OS::sc_StartThread()
{
	uint32 id	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 arg	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	THREAD* thread = GetThread(id);
	if(!thread->valid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	assert(thread->status == THREAD_ZOMBIE);
	thread->status = THREAD_RUNNING;
	thread->epc = thread->threadProc;

	auto context = reinterpret_cast<THREADCONTEXT*>(&m_ram[thread->contextPtr]);
	context->gpr[CMIPS::A0].nV0 = arg;
	context->gpr[CMIPS::RA].nV0 = BIOS_ADDRESS_THREADEPILOG;
	context->gpr[CMIPS::SP].nV0 = thread->contextPtr;
	context->gpr[CMIPS::FP].nV0 = thread->contextPtr;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = id;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

	ThreadShakeAndBake();
}

//23
void CPS2OS::sc_ExitThread()
{
	uint32 threadId = GetCurrentThreadId();

	auto thread = GetThread(threadId);
	thread->status = THREAD_ZOMBIE;

	//Reset the thread's priority
	thread->currPriority = thread->initPriority;
	m_threadSchedule->Remove(thread->scheduleID);
	thread->scheduleID = m_threadSchedule->Insert(threadId, thread->currPriority);

	ThreadShakeAndBake();
}

//24
void CPS2OS::sc_ExitDeleteThread()
{
	auto thread = GetThread(GetCurrentThreadId());
	thread->status = THREAD_ZOMBIE;
	
	ThreadShakeAndBake();

	m_threadSchedule->Remove(thread->scheduleID);
	thread->valid = 0;
}

//25
void CPS2OS::sc_TerminateThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	if(id == GetCurrentThreadId())
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(id >= MAX_THREAD)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	auto thread = GetThread(id);
	if(!thread || !thread->valid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(thread->status == THREAD_ZOMBIE)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	thread->status = THREAD_ZOMBIE;

	//Reset the thread's priority
	thread->currPriority = thread->initPriority;
	m_threadSchedule->Remove(thread->scheduleID);
	thread->scheduleID = m_threadSchedule->Insert(id, thread->currPriority);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(id);
}

//29
//2A
void CPS2OS::sc_ChangeThreadPriority()
{
	bool isInt		= m_ee.m_State.nGPR[3].nV[0] == 0x2A;
	uint32 id		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 prio		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	THREAD* thread = GetThread(id);
	if(!thread->valid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nV[0] = 0xFFFFFFFF;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0xFFFFFFFF;
		return;
	}

	uint32 prevPrio = thread->currPriority;
	thread->currPriority = prio;

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = prevPrio;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

	//Reschedule?
	m_threadSchedule->Remove(thread->scheduleID);
	thread->scheduleID = m_threadSchedule->Insert(id, thread->currPriority);

	if(!isInt)
	{
		ThreadShakeAndBake();
	}
}

//2B
void CPS2OS::sc_RotateThreadReadyQueue()
{
	CRoundRibbon::ITERATOR threadIterator(m_threadSchedule);

	uint32 prio = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	//TODO: Rescheduling isn't always necessary and will cause the current thread's priority queue to be
	//rotated too since each time a thread is picked to be executed it's placed at the end of the queue...

	//Find first of this priority and reinsert if it's the same as the current thread
	//If it's not the same, the schedule will be rotated when another thread is choosen
	for(threadIterator = m_threadSchedule->Begin(); !threadIterator.IsEnd(); threadIterator++)
	{
		if(threadIterator.GetWeight() == prio)
		{
			uint32 id = threadIterator.GetValue();
			if(id == GetCurrentThreadId())
			{
				//TODO: Need to verify that
				THREAD* thread(GetThread(id));
				m_threadSchedule->Remove(threadIterator.GetIndex());
				thread->scheduleID = m_threadSchedule->Insert(id, prio);
			}
			break;
		}
	}

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = prio;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

	if(!threadIterator.IsEnd())
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
	uint32 id			= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 statusPtr	= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	if(id >= MAX_THREAD)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	if(id == 0)
	{
		id = GetCurrentThreadId();
	}

	auto thread = GetThread(id);
	if(!thread->valid)
	{
		//TODO: This is actually valid on a real PS2 and won't return an error
		m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(-1);
		return;
	}

	uint32 ret = 0;
	switch(thread->status)
	{
	case THREAD_RUNNING:
		if(id == GetCurrentThreadId())
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

	if(statusPtr != 0)
	{
		auto threadParam = reinterpret_cast<THREADPARAM*>(GetStructPtr(statusPtr));

		threadParam->status				= ret;
		threadParam->initPriority		= thread->initPriority;
		threadParam->currPriority		= thread->currPriority;
		threadParam->stackBase			= thread->stackBase;
		threadParam->stackSize			= thread->stackSize;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = ret;
}

//32
void CPS2OS::sc_SleepThread()
{
	THREAD* thread = GetThread(GetCurrentThreadId());
	if(thread->wakeUpCount == 0)
	{
		assert(thread->status == THREAD_RUNNING);
		thread->status = THREAD_SLEEPING;
		ThreadShakeAndBake();
		return;
	}

	thread->wakeUpCount--;
}

//33
//34
void CPS2OS::sc_WakeupThread()
{
	uint32 id		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	bool isInt		= m_ee.m_State.nGPR[3].nV[0] == 0x34;

	THREAD* thread = GetThread(id);

	if(
		(thread->status == THREAD_SLEEPING) || 
		(thread->status == THREAD_SUSPENDED_SLEEPING))
	{
		switch(thread->status)
		{
		case THREAD_SLEEPING:
			thread->status = THREAD_RUNNING;
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
	uint32 id		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	bool isInt		= m_ee.m_State.nGPR[3].nV[0] == 0x36;

	auto thread = GetThread(id);
	if(!thread->valid)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = ~0ULL;
		return;
	}

	uint32 wakeUpCount = thread->wakeUpCount;
	thread->wakeUpCount = 0;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = wakeUpCount;
}

//37
void CPS2OS::sc_SuspendThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	
	THREAD* thread = GetThread(id);
	if(!thread->valid)
	{
		return;
	}

	switch(thread->status)
	{
	case THREAD_RUNNING:
		thread->status = THREAD_SUSPENDED;
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

	ThreadShakeAndBake();
}

//39
void CPS2OS::sc_ResumeThread()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	THREAD* thread = GetThread(id);
	if(!thread->valid)
	{
		return;
	}

	switch(thread->status)
	{
	case THREAD_SUSPENDED:
		thread->status = THREAD_RUNNING;
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

	ThreadShakeAndBake();
}

//3C
void CPS2OS::sc_SetupThread()
{
	uint32 stackBase = m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	uint32 stackSize = m_ee.m_State.nGPR[SC_PARAM2].nV[0];

	uint32 stackAddr = 0;
	if(stackBase == 0xFFFFFFFF)
	{
		stackAddr = 0x02000000;
	}
	else
	{
		stackAddr = stackBase + stackSize;
	}

	uint32 argsBase = m_ee.m_State.nGPR[SC_PARAM3].nV[0];
	//Copy arguments
	{
		ArgumentList completeArgList;
		completeArgList.push_back(m_executableName);
		completeArgList.insert(completeArgList.end(), m_currentArguments.begin(), m_currentArguments.end());

		uint32 argsCount = static_cast<uint32>(completeArgList.size());

		*reinterpret_cast<uint32*>(m_ram + argsBase) = argsCount;
		uint32 argsPtrs = argsBase + 4;
		//We add 1 to argsCount because argv[argc] must be equal to 0
		uint32 argsPayload = argsPtrs + ((argsCount + 1) * 4);
		for(uint32 i = 0; i < argsCount; i++)
		{
			const auto& currentArg = completeArgList[i];
			*reinterpret_cast<uint32*>(m_ram + argsPtrs + (i * 4)) = argsPayload;
			uint32 argSize = static_cast<uint32>(currentArg.size()) + 1;
			memcpy(m_ram + argsPayload, currentArg.c_str(), argSize);
			argsPayload += argSize;
		}
		//Set argv[argc] to 0
		*reinterpret_cast<uint32*>(m_ram + argsPtrs + (argsCount * 4)) = 0;
	}

	//Set up the main thread
	auto thread = GetThread(1);
	thread->valid			= 0x01;
	thread->status			= THREAD_RUNNING;
	thread->stackBase		= stackAddr - stackSize;
	thread->initPriority	= 0;
	thread->currPriority	= 0;
	thread->quota			= THREAD_INIT_QUOTA;
	thread->scheduleID		= m_threadSchedule->Insert(1, thread->currPriority);
	thread->contextPtr		= 0;

	SetCurrentThreadId(1);

	m_ee.m_State.nGPR[SC_RETURN].nV[0] = stackAddr;
	m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
}

//3D
void CPS2OS::sc_SetupHeap()
{
	THREAD* thread = GetThread(GetCurrentThreadId());

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
	THREAD* thread = GetThread(GetCurrentThreadId());

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
	sema->count			= semaParam->initCount;
	sema->maxCount		= semaParam->maxCount;
	sema->waitCount		= 0;

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
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	//Check if any threads are waiting for this?
	if(sema->waitCount != 0)
	{
		assert(0);
	}

	m_semaphores.Free(id);

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//42
//43
void CPS2OS::sc_SignalSema()
{
	bool isInt	= m_ee.m_State.nGPR[3].nV[0] == 0x43;
	uint32 id	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}
	
	if(sema->waitCount != 0)
	{
		//Unsleep all threads if they were waiting
		for(uint32 i = 0; i < MAX_THREAD; i++)
		{
			THREAD* thread = GetThread(i);
			if(!thread->valid) continue;
			if((thread->status != THREAD_WAITING) && (thread->status != THREAD_SUSPENDED_WAITING)) continue;
			if(thread->semaWait != id) continue;

			switch(thread->status)
			{
			case THREAD_WAITING:
				thread->status = THREAD_RUNNING;
				break;
			case THREAD_SUSPENDED_WAITING:
				thread->status = THREAD_SUSPENDED;
				break;
			default:
				assert(0);
				break;
			}
			thread->quota = THREAD_INIT_QUOTA;
			sema->waitCount--;

			if(sema->waitCount == 0)
			{
				break;
			}
		}

		m_ee.m_State.nGPR[SC_RETURN].nV[0] = id;
		m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;

		if(!isInt)
		{
			ThreadShakeAndBake();
		}
	}
	else
	{
		sema->count++;
	}

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//44
void CPS2OS::sc_WaitSema()
{
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	if((m_semaWaitId == id) && (m_semaWaitCaller == m_ee.m_State.nGPR[CMIPS::RA].nV0))
	{
		m_semaWaitCount++;
		if(m_semaWaitCount > 100)
		{
			m_semaWaitThreadId = GetCurrentThreadId();
		}
	}
	else
	{
		m_semaWaitId = id;
		m_semaWaitCaller = m_ee.m_State.nGPR[CMIPS::RA].nV0;
		m_semaWaitCount = 0;
	}

	if(sema->count == 0)
	{
		//Put this thread in sleep mode and reschedule...
		sema->waitCount++;

		THREAD* thread = GetThread(GetCurrentThreadId());
		assert(thread->status == THREAD_RUNNING);
		thread->status		= THREAD_WAITING;
		thread->semaWait	= id;

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
	bool isInt = m_ee.m_State.nGPR[3].nV[0] != 0x47;
	uint32 id = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	SEMAPHOREPARAM* semaParam = (SEMAPHOREPARAM*)(m_ram + (m_ee.m_State.nGPR[SC_PARAM1].nV[0] & 0x1FFFFFFF));

	auto sema = m_semaphores[id];
	if(sema == nullptr)
	{
		m_ee.m_State.nGPR[SC_RETURN].nD0 = -1;
		return;
	}

	semaParam->count		= sema->count;
	semaParam->maxCount		= sema->maxCount;
	semaParam->waitThreads	= sema->waitCount;

	m_ee.m_State.nGPR[SC_RETURN].nD0 = id;
}

//64
void CPS2OS::sc_FlushCache()
{
	uint32 operationType = m_ee.m_State.nGPR[SC_PARAM0].nV[0];
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
		m_gs->WritePrivRegister(CGSHandler::GS_IMR, imr);
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

		uint32 handlerId = GetNextAvailableIntcHandlerId();
		if(handlerId == 0xFFFFFFFF)
		{
			CLog::GetInstance().Print(LOG_NAME, "Couldn't set INTC handler through SetSyscall");
			return;
		}

		INTCHANDLER* handler = GetIntcHandler(handlerId);
		handler->valid		= 1;
		handler->address	= address & 0x1FFFFFFF;
		handler->cause		= line;
		handler->arg		= 0;
		handler->gp			= 0;

		if(!(m_ee.m_pMemoryMap->GetWord(CINTC::INTC_MASK) & (1 << line)))
		{
			m_ee.m_pMemoryMap->SetWord(CINTC::INTC_MASK, (1 << line));
		}
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Unknown syscall set.", number);
	}

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
	auto xfer = reinterpret_cast<SIFDMAREG*>(GetStructPtr(m_ee.m_State.nGPR[SC_PARAM0].nV0));
	uint32 count = m_ee.m_State.nGPR[SC_PARAM1].nV[0];

	//Returns count
	//DMA might call an interrupt handler
	m_ee.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(count);

	for(unsigned int i = 0; i < count; i++)
	{
		uint32 size = (xfer[i].size + 0x0F) / 0x10;

		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_MADR,	xfer[i].srcAddr);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_TADR,	xfer[i].dstAddr);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_QWC,	size);
		m_ee.m_pMemoryMap->SetWord(CDMAC::D6_CHCR,	0x00000100);
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
	uint32 registerId	= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 value		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];

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
	uint32 function		= m_ee.m_State.nGPR[SC_PARAM0].nV[0];
	uint32 param		= m_ee.m_State.nGPR[SC_PARAM1].nV[0];
	
	switch(function)
	{
	case 0x01:
		//Deci2Open
		{
			uint32 id = GetNextAvailableDeci2HandlerId();

			DECI2HANDLER* handler = GetDeci2Handler(id);
			handler->valid		= 1;
			handler->device		= *(uint32*)&m_ram[param + 0x00];
			handler->bufferAddr	= *(uint32*)&m_ram[param + 0x04];

			m_ee.m_State.nGPR[SC_RETURN].nV[0] = id;
			m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
		}
		break;
	case 0x03:
		//Deci2Send
		{
			uint32 id = *reinterpret_cast<uint32*>(&m_ram[param + 0x00]);

			DECI2HANDLER* handler = GetDeci2Handler(id);

			if(handler->valid != 0)
			{
				uint32 stringAddr = *reinterpret_cast<uint32*>(&m_ram[handler->bufferAddr + 0x10]);
				stringAddr &= (PS2::EE_RAM_SIZE - 1);

				uint32 length = m_ram[stringAddr + 0x00] - 0x0C;
				uint8* string = &m_ram[stringAddr + 0x0C];

				m_iopBios.GetIoman()->Write(Iop::CIoman::FID_STDOUT, length, string);
			}

			m_ee.m_State.nGPR[SC_RETURN].nV[0] = 1;
			m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
		}
		break;
	case 0x04:
		//Deci2Poll
		{
			uint32 id = *reinterpret_cast<uint32*>(&m_ram[param + 0x00]);
		
			DECI2HANDLER* handler = GetDeci2Handler(id);
			if(handler->valid != 0)
			{
				*(uint32*)&m_ram[handler->bufferAddr + 0x0C] = 0;
			}

			m_ee.m_State.nGPR[SC_RETURN].nV[0] = 1;
			m_ee.m_State.nGPR[SC_RETURN].nV[1] = 0;
		}
		break;
	case 0x10:
		//kPuts
		{
			uint32 stringAddr = *reinterpret_cast<uint32*>(&m_ram[param]);
			uint8* string = &m_ram[stringAddr];
			m_iopBios.GetIoman()->Write(1, static_cast<uint32>(strlen(reinterpret_cast<char*>(string))), string);
		}
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown Deci2Call function (0x%0.8X) called. PC: 0x%0.8X.\r\n", function, m_ee.m_State.nPC);
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
	m_ee.m_State.nGPR[SC_RETURN].nV[0] = PS2::EE_RAM_SIZE;
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
		CLog::GetInstance().Print(LOG_NAME, "System call exception occured but no SYSCALL instruction found (addr = 0x%0.8X, opcode = 0x%0.8X).\r\n",
			searchAddress, callInstruction);
		m_ee.m_State.nHasException = 0;
		return;
	}

	uint32 func = m_ee.m_State.nGPR[3].nV[0];
	
	if(func == 0x666)
	{
		//Reschedule
		ThreadShakeAndBake();
	}
	else
	{
		if(func & 0x80000000)
		{
			func = 0 - func;
		}
		//Save for custom handler
		m_ee.m_State.nGPR[3].nV[0] = func;

		if(GetCustomSyscallTable()[func] == NULL)
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

	m_ee.m_State.nHasException = 0;
}

void CPS2OS::DisassembleSysCall(uint8 func)
{
#ifdef _DEBUG
	std::string description(GetSysCallDescription(func));
	if(description.length() != 0)
	{
		CLog::GetInstance().Print(LOG_NAME, "%d: %s\r\n", GetCurrentThreadId(), description.c_str());
	}
#endif
}

std::string CPS2OS::GetSysCallDescription(uint8 function)
{
	char description[256];

	strcpy(description, "");

	switch(function)
	{
	case 0x02:
		sprintf(description, "GsSetCrt(interlace = %i, mode = %i, field = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x04:
		sprintf(description, SYSCALL_NAME_EXIT "();");
		break;
	case 0x06:
		sprintf(description, SYSCALL_NAME_LOADEXECPS2 "(exec = 0x%0.8X, argc = %d, argv = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0],
			m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x07:
		sprintf(description, SYSCALL_NAME_EXECPS2 "(pc = 0x%0.8X, gp = 0x%0.8X, argc = %d, argv = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM0].nV[1],
			m_ee.m_State.nGPR[SC_PARAM0].nV[2],
			m_ee.m_State.nGPR[SC_PARAM0].nV[3]);
		break;
	case 0x10:
		sprintf(description, SYSCALL_NAME_ADDINTCHANDLER "(cause = %i, address = 0x%0.8X, next = 0x%0.8X, arg = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0],
			m_ee.m_State.nGPR[SC_PARAM2].nV[0],
			m_ee.m_State.nGPR[SC_PARAM3].nV[0]);
		break;
	case 0x11:
		sprintf(description, SYSCALL_NAME_REMOVEINTCHANDLER "(cause = %i, id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x12:
		sprintf(description, SYSCALL_NAME_ADDDMACHANDLER "(channel = %i, address = 0x%0.8X, next = %i, arg = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM2].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM3].nV[0]);
		break;
	case 0x13:
		sprintf(description, SYSCALL_NAME_REMOVEDMACHANDLER "(channel = %i, handler = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x14:
		sprintf(description, SYSCALL_NAME_ENABLEINTC "(cause = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x15:
		sprintf(description, SYSCALL_NAME_DISABLEINTC "(cause = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x16:
		sprintf(description, SYSCALL_NAME_ENABLEDMAC "(channel = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x17:
		sprintf(description, SYSCALL_NAME_DISABLEDMAC "(channel = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x18:
		sprintf(description, SYSCALL_NAME_SETALARM "(time = %d, proc = 0x%0.8X, arg = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], 
			m_ee.m_State.nGPR[SC_PARAM1].nV[0], 
			m_ee.m_State.nGPR[SC_PARAM2].nV[0]);
		break;
	case 0x1F:
		sprintf(description, SYSCALL_NAME_IRELEASEALARM "(id = %d);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x20:
		sprintf(description, SYSCALL_NAME_CREATETHREAD "(thread = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x21:
		sprintf(description, SYSCALL_NAME_DELETETHREAD "(id = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x22:
		sprintf(description, SYSCALL_NAME_STARTTHREAD "(id = 0x%0.8X, a0 = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x23:
		sprintf(description, SYSCALL_NAME_EXITTHREAD "();");
		break;
	case 0x24:
		sprintf(description, SYSCALL_NAME_EXITDELETETHREAD "();");
		break;
	case 0x25:
		sprintf(description, SYSCALL_NAME_TERMINATETHREAD "(id = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x29:
		sprintf(description, SYSCALL_NAME_CHANGETHREADPRIORITY "(id = 0x%0.8X, priority = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x2A:
		sprintf(description, SYSCALL_NAME_ICHANGETHREADPRIORITY "(id = 0x%0.8X, priority = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x2B:
		sprintf(description, SYSCALL_NAME_ROTATETHREADREADYQUEUE "(prio = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x2F:
		sprintf(description, SYSCALL_NAME_GETTHREADID "();");
		break;
	case 0x30:
		sprintf(description, SYSCALL_NAME_REFERTHREADSTATUS "(threadId = %d, infoPtr = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x31:
		sprintf(description, SYSCALL_NAME_IREFERTHREADSTATUS "(threadId = %d, infoPtr = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x32:
		sprintf(description, SYSCALL_NAME_SLEEPTHREAD "();");
		break;
	case 0x33:
		sprintf(description, SYSCALL_NAME_WAKEUPTHREAD "(id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x34:
		sprintf(description, SYSCALL_NAME_IWAKEUPTHREAD "(id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x35:
		sprintf(description, SYSCALL_NAME_CANCELWAKEUPTHREAD "(id = %d);", 
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x36:
		sprintf(description, SYSCALL_NAME_ICANCELWAKEUPTHREAD "(id = %d);", 
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x37:
		sprintf(description, SYSCALL_NAME_SUSPENDTHREAD "(id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x39:
		sprintf(description, SYSCALL_NAME_RESUMETHREAD "(id = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x3C:
		sprintf(description, "SetupThread(gp = 0x%0.8X, stack = 0x%0.8X, stack_size = 0x%0.8X, args = 0x%0.8X, root_func = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM2].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM3].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM4].nV[0]);
		break;
	case 0x3D:
		sprintf(description, "SetupHeap(heap_start = 0x%0.8X, heap_size = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x3E:
		sprintf(description, SYSCALL_NAME_ENDOFHEAP "();");
		break;
	case 0x40:
		sprintf(description, SYSCALL_NAME_CREATESEMA "(sema = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x41:
		sprintf(description, SYSCALL_NAME_DELETESEMA "(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x42:
		sprintf(description, SYSCALL_NAME_SIGNALSEMA "(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x43:
		sprintf(description, SYSCALL_NAME_ISIGNALSEMA "(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x44:
		sprintf(description, SYSCALL_NAME_WAITSEMA "(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x45:
		sprintf(description, SYSCALL_NAME_POLLSEMA "(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x46:
		sprintf(description, "iPollSema(semaid = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x47:
		sprintf(description, SYSCALL_NAME_REFERSEMASTATUS "(semaid = %i, status = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x48:
		sprintf(description, SYSCALL_NAME_IREFERSEMASTATUS "(semaid = %i, status = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x64:
	case 0x68:
#ifdef _DEBUG
//		sprintf(description, SYSCALL_NAME_FLUSHCACHE "();");
#endif
		break;
	case 0x70:
		sprintf(description, SYSCALL_NAME_GSGETIMR "();");
		break;
	case 0x71:
		sprintf(description, SYSCALL_NAME_GSPUTIMR "(GS_IMR = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x73:
		sprintf(description, SYSCALL_NAME_SETVSYNCFLAG "(ptr1 = 0x%0.8X, ptr2 = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x74:
		sprintf(description, "SetSyscall(num = 0x%0.2X, address = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x76:
		sprintf(description, SYSCALL_NAME_SIFDMASTAT "();");
		break;
	case 0x77:
		sprintf(description, SYSCALL_NAME_SIFSETDMA "(list = 0x%0.8X, count = %i);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x78:
		sprintf(description, SYSCALL_NAME_SIFSETDCHAIN "();");
		break;
	case 0x79:
		sprintf(description, "SifSetReg(register = 0x%0.8X, value = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0], \
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7A:
		sprintf(description, "SifGetReg(register = 0x%0.8X);", \
			m_ee.m_State.nGPR[SC_PARAM0].nV[0]);
		break;
	case 0x7C:
		sprintf(description, SYSCALL_NAME_DECI2CALL "(func = 0x%0.8X, param = 0x%0.8X);",
			m_ee.m_State.nGPR[SC_PARAM0].nV[0],
			m_ee.m_State.nGPR[SC_PARAM1].nV[0]);
		break;
	case 0x7E:
		sprintf(description, SYSCALL_NAME_MACHINETYPE "();");
		break;
	case 0x7F:
		sprintf(description, "GetMemorySize();");
		break;
	}

	return std::string(description);
}

//////////////////////////////////////////////////
//System Call Handlers Table
//////////////////////////////////////////////////

CPS2OS::SystemCallHandler CPS2OS::m_sysCall[0x80] =
{
	//0x00
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_GsSetCrt,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Exit,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_LoadExecPS2,		&CPS2OS::sc_ExecPS2,
	//0x08
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x10
	&CPS2OS::sc_AddIntcHandler,		&CPS2OS::sc_RemoveIntcHandler,		&CPS2OS::sc_AddDmacHandler,			&CPS2OS::sc_RemoveDmacHandler,		&CPS2OS::sc_EnableIntc,			&CPS2OS::sc_DisableIntc,		&CPS2OS::sc_EnableDmac,			&CPS2OS::sc_DisableDmac,
	//0x18
	&CPS2OS::sc_SetAlarm,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_ReleaseAlarm,
	//0x20
	&CPS2OS::sc_CreateThread,		&CPS2OS::sc_DeleteThread,			&CPS2OS::sc_StartThread,			&CPS2OS::sc_ExitThread,				&CPS2OS::sc_ExitDeleteThread,	&CPS2OS::sc_TerminateThread,	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x28
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_ChangeThreadPriority,	&CPS2OS::sc_ChangeThreadPriority,	&CPS2OS::sc_RotateThreadReadyQueue,	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_GetThreadId,
	//0x30
	&CPS2OS::sc_ReferThreadStatus,	&CPS2OS::sc_ReferThreadStatus,		&CPS2OS::sc_SleepThread,			&CPS2OS::sc_WakeupThread,			&CPS2OS::sc_WakeupThread,		&CPS2OS::sc_CancelWakeupThread,	&CPS2OS::sc_CancelWakeupThread,	&CPS2OS::sc_SuspendThread,
	//0x38
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_ResumeThread,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_SetupThread,		&CPS2OS::sc_SetupHeap,			&CPS2OS::sc_EndOfHeap,			&CPS2OS::sc_Unhandled,
	//0x40
	&CPS2OS::sc_CreateSema,			&CPS2OS::sc_DeleteSema,				&CPS2OS::sc_SignalSema,				&CPS2OS::sc_SignalSema,				&CPS2OS::sc_WaitSema,			&CPS2OS::sc_PollSema,			&CPS2OS::sc_PollSema,			&CPS2OS::sc_ReferSemaStatus,
	//0x48
	&CPS2OS::sc_ReferSemaStatus,	&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x50
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x58
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x60
	&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_FlushCache,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x68
	&CPS2OS::sc_FlushCache,			&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_Unhandled,
	//0x70
	&CPS2OS::sc_GsGetIMR,			&CPS2OS::sc_GsPutIMR,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_SetVSyncFlag,			&CPS2OS::sc_SetSyscall,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_SifDmaStat,			&CPS2OS::sc_SifSetDma,
	//0x78
	&CPS2OS::sc_SifSetDChain,		&CPS2OS::sc_SifSetReg,				&CPS2OS::sc_SifGetReg,				&CPS2OS::sc_Unhandled,				&CPS2OS::sc_Deci2Call,			&CPS2OS::sc_Unhandled,			&CPS2OS::sc_MachineType,		&CPS2OS::sc_GetMemorySize,
};

//////////////////////////////////////////////////
//Round Ribbon Implementation
//////////////////////////////////////////////////

CPS2OS::CRoundRibbon::CRoundRibbon(void* memory, uint32 size)
: m_node(reinterpret_cast<NODE*>(memory))
, m_maxNode(size / sizeof(NODE))
{
	memset(memory, 0, size);

	NODE* head = GetNode(0);
	head->indexNext	= -1;
	head->weight	= -1;
	head->valid		= 1;
}

CPS2OS::CRoundRibbon::~CRoundRibbon()
{

}

unsigned int CPS2OS::CRoundRibbon::Insert(uint32 value, uint32 weight)
{
	//Initialize the new node
	NODE* node = AllocateNode();
	if(node == NULL) return -1;
	node->weight	= weight;
	node->value		= value;

	//Insert node in list
	NODE* next = GetNode(0);
	NODE* prev = NULL;

	while(1)
	{
		if(next == NULL)
		{
			//We must insert there...
			node->indexNext = prev->indexNext;
			prev->indexNext = GetNodeIndex(node);
			break;
		}

		if(next->weight == -1)
		{
			prev = next;
			next = GetNode(next->indexNext);
			continue;
		}

		if(node->weight < next->weight)
		{
			next = NULL;
			continue;
		}

		prev = next;
		next = GetNode(next->indexNext);
	}

	return GetNodeIndex(node);
}

void CPS2OS::CRoundRibbon::Remove(unsigned int index)
{
	if(index == 0) return;

	NODE* curr = GetNode(index);
	if(curr == NULL) return;
	if(curr->valid != 1) return;

	NODE* node = GetNode(0);

	while(1)
	{
		if(node == NULL) break;
		assert(node->valid);

		if(node->indexNext == index)
		{
			node->indexNext = curr->indexNext;
			break;
		}
		
		node = GetNode(node->indexNext);
	}

	FreeNode(curr);
}

unsigned int CPS2OS::CRoundRibbon::Begin()
{
	return GetNode(0)->indexNext;
}

CPS2OS::CRoundRibbon::NODE* CPS2OS::CRoundRibbon::GetNode(unsigned int index)
{
	if(index >= m_maxNode) return NULL;
	return m_node + index;
}

unsigned int CPS2OS::CRoundRibbon::GetNodeIndex(NODE* node)
{
	return (unsigned int)(node - m_node);
}

CPS2OS::CRoundRibbon::NODE* CPS2OS::CRoundRibbon::AllocateNode()
{
	for(unsigned int i = 1; i < m_maxNode; i++)
	{
		NODE* node = GetNode(i);
		if(node->valid == 1) continue;
		node->valid = 1;
		return node;
	}

	return NULL;
}

void CPS2OS::CRoundRibbon::FreeNode(NODE* node)
{
	node->valid = 0;
}

CPS2OS::CRoundRibbon::ITERATOR::ITERATOR(CRoundRibbon* pRibbon)
: m_ribbon(pRibbon)
, m_index(0)
{

}

CPS2OS::CRoundRibbon::ITERATOR& CPS2OS::CRoundRibbon::ITERATOR::operator =(unsigned int index)
{
	m_index = index;
	return (*this);
}

CPS2OS::CRoundRibbon::ITERATOR& CPS2OS::CRoundRibbon::ITERATOR::operator ++(int)
{
	if(!IsEnd())
	{
		NODE* node = m_ribbon->GetNode(m_index);
		m_index = node->indexNext;
	}

	return (*this);
}

uint32 CPS2OS::CRoundRibbon::ITERATOR::GetValue()
{
	if(!IsEnd())
	{
		return m_ribbon->GetNode(m_index)->value;
	}

	return 0;
}

uint32 CPS2OS::CRoundRibbon::ITERATOR::GetWeight()
{
	if(!IsEnd())
	{
		return m_ribbon->GetNode(m_index)->weight;
	}

	return -1;
}

unsigned int CPS2OS::CRoundRibbon::ITERATOR::GetIndex()
{
	return m_index;
}

bool CPS2OS::CRoundRibbon::ITERATOR::IsEnd()
{
	if(m_ribbon == NULL) return true;
	return m_ribbon->GetNode(m_index) == NULL;
}

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
		module.name		= m_executableName;
		module.begin	= executableRange.first;
		module.end		= executableRange.second;
		module.param	= m_elf;
		result.push_back(module);
	}

	return result;
}

BiosDebugThreadInfoArray CPS2OS::GetThreadsDebugInfo() const
{
	BiosDebugThreadInfoArray threadInfos;

	CRoundRibbon::ITERATOR threadIterator(m_threadSchedule);

	for(threadIterator = m_threadSchedule->Begin(); 
		!threadIterator.IsEnd(); threadIterator++)
	{
		auto thread = GetThread(threadIterator.GetValue());
		auto threadContext = reinterpret_cast<THREADCONTEXT*>(GetStructPtr(thread->contextPtr));

		BIOS_DEBUG_THREAD_INFO threadInfo;
		threadInfo.id			= threadIterator.GetValue();
		threadInfo.priority		= thread->currPriority;
		if(GetCurrentThreadId() == threadIterator.GetValue())
		{
			threadInfo.pc = m_ee.m_State.nPC;
			threadInfo.ra = m_ee.m_State.nGPR[CMIPS::RA].nV0;
			threadInfo.sp = m_ee.m_State.nGPR[CMIPS::SP].nV0;
		}
		else
		{
			threadInfo.pc = thread->epc;
			threadInfo.ra = threadContext->gpr[CMIPS::RA].nV0;
			threadInfo.sp = threadContext->gpr[CMIPS::SP].nV0;
		}

		switch(thread->status)
		{
		case THREAD_RUNNING:
			threadInfo.stateDescription = "Running";
			break;
		case THREAD_SLEEPING:
			threadInfo.stateDescription = "Sleeping";
			break;
		case THREAD_WAITING:
			threadInfo.stateDescription = "Waiting (Semaphore: " + boost::lexical_cast<std::string>(thread->semaWait) + ")";
			break;
		case THREAD_SUSPENDED:
			threadInfo.stateDescription = "Suspended";
			break;
		case THREAD_SUSPENDED_SLEEPING:
			threadInfo.stateDescription = "Suspended+Sleeping";
			break;
		case THREAD_SUSPENDED_WAITING:
			threadInfo.stateDescription = "Suspended+Waiting (Semaphore: " + boost::lexical_cast<std::string>(thread->semaWait) + ")";
			break;
		case THREAD_ZOMBIE:
			threadInfo.stateDescription = "Zombie";
			break;
		default:
			threadInfo.stateDescription = "Unknown";
			break;
		}

		threadInfos.push_back(threadInfo);
	}

	return threadInfos;
}

#endif
