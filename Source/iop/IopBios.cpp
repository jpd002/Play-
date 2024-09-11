#include <vector>
#include <cstring>

#include "string_format.h"
#include "PtrStream.h"
#include "xml/FilteringNodeIterator.h"
#include "lexical_cast_ex.h"
#include "BitManip.h"
#include "StringUtils.h"
#include "std_experimental_map.h"

#include "IopBios.h"
#include "../COP_SCU.h"
#include "../Log.h"
#include "../ElfFile.h"
#include "../Ps2Const.h"
#include "../MipsExecutor.h"
#include "Iop_Intc.h"
#include "../states/MemoryStateFile.h"
#include "../states/RegisterStateCollectionFile.h"

#ifdef _IOP_EMULATE_MODULES
#include "Iop_IomanX.h"
#include "Iop_Naplink.h"
#endif

#include "Iop_SifManNull.h"
#include "Iop_SifModuleProvider.h"
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include "Iop_Thbase.h"
#include "Iop_Thsema.h"
#include "Iop_Thfpool.h"
#include "Iop_Thvpool.h"
#include "Iop_Thmsgbx.h"
#include "Iop_Thevent.h"
#include "Iop_Heaplib.h"
#include "Iop_Timrman.h"
#include "Iop_Intrman.h"
#include "Iop_Dmacman.h"
#include "Iop_Secrman.h"
#include "Iop_Vblank.h"
#include "Iop_Dynamic.h"

#include "Ioman_ScopedFile.h"

#define LOGNAME "iop_bios"

#define STATE_MODULES ("iopbios/dyn_modules.xml")
#define STATE_MODULE_IMPORT_TABLE_ADDRESS ("ImportTableAddress")

#define STATE_MODULESTARTREQUESTS ("iopbios/module_start_requests")

#define BIOS_THREAD_LINK_HEAD_BASE (CIopBios::CONTROL_BLOCK_START + 0x0000)
#define BIOS_CURRENT_THREAD_ID_BASE (CIopBios::CONTROL_BLOCK_START + 0x0008)
#define BIOS_CURRENT_TIME_BASE (CIopBios::CONTROL_BLOCK_START + 0x0010)
#define BIOS_MODULESTARTREQUEST_HEAD_BASE (CIopBios::CONTROL_BLOCK_START + 0x0018)
#define BIOS_MODULESTARTREQUEST_FREE_BASE (CIopBios::CONTROL_BLOCK_START + 0x0020)
#define BIOS_HANDLERS_BASE (CIopBios::CONTROL_BLOCK_START + 0x0100)
#define BIOS_HANDLERS_END (BIOS_HANDLERS_BASE + 0x100 - 1)
#define BIOS_MESSAGEBOX_BASE (BIOS_HANDLERS_END + 1)
#define BIOS_MESSAGEBOX_SIZE (sizeof(CIopBios::MESSAGEBOX) * CIopBios::MAX_MESSAGEBOX)
#define BIOS_SYSTEM_INTRHANDLER_TABLE_BASE (0x480)
#define BIOS_SYSTEM_INTRHANDLER_TABLE_SIZE (0x3 * (sizeof(CIopBios::SYSTEM_INTRHANDLER)))
#define BIOS_THREADS_BASE (BIOS_SYSTEM_INTRHANDLER_TABLE_BASE + BIOS_SYSTEM_INTRHANDLER_TABLE_SIZE)
#define BIOS_THREADS_SIZE (sizeof(CIopBios::THREAD) * CIopBios::MAX_THREAD)
#define BIOS_SEMAPHORES_BASE (BIOS_THREADS_BASE + BIOS_THREADS_SIZE)
#define BIOS_SEMAPHORES_SIZE (sizeof(CIopBios::SEMAPHORE) * CIopBios::MAX_SEMAPHORE)
#define BIOS_EVENTFLAGS_BASE (BIOS_SEMAPHORES_BASE + BIOS_SEMAPHORES_SIZE)
#define BIOS_EVENTFLAGS_SIZE (sizeof(CIopBios::EVENTFLAG) * CIopBios::MAX_EVENTFLAG)
#define BIOS_INTRHANDLER_BASE (BIOS_EVENTFLAGS_BASE + BIOS_EVENTFLAGS_SIZE)
#define BIOS_INTRHANDLER_SIZE (sizeof(CIopBios::INTRHANDLER) * CIopBios::MAX_INTRHANDLER)
#define BIOS_VBLANKHANDLER_BASE (BIOS_INTRHANDLER_BASE + BIOS_INTRHANDLER_SIZE)
#define BIOS_VBLANKHANDLER_SIZE (sizeof(CIopBios::VBLANKHANDLER) * CIopBios::MAX_VBLANKHANDLER)
#define BIOS_FPL_BASE (BIOS_VBLANKHANDLER_BASE + BIOS_VBLANKHANDLER_SIZE)
#define BIOS_FPL_SIZE (sizeof(CIopBios::FPL) * CIopBios::MAX_FPL)
#define BIOS_VPL_BASE (BIOS_FPL_BASE + BIOS_FPL_SIZE)
#define BIOS_VPL_SIZE (sizeof(CIopBios::VPL) * CIopBios::MAX_VPL)
#define BIOS_MEMORYBLOCK_BASE (BIOS_VPL_BASE + BIOS_VPL_SIZE)
#define BIOS_MEMORYBLOCK_SIZE (sizeof(Iop::MEMORYBLOCK) * CIopBios::MAX_MEMORYBLOCK)
#define BIOS_LOADEDMODULE_BASE (BIOS_MEMORYBLOCK_BASE + BIOS_MEMORYBLOCK_SIZE)
#define BIOS_LOADEDMODULE_SIZE (sizeof(CIopBios::LOADEDMODULE) * CIopBios::MAX_LOADEDMODULE)
#define BIOS_INTSTACK_BASE (BIOS_LOADEDMODULE_BASE + BIOS_LOADEDMODULE_SIZE)
#define BIOS_INTSTACK_SIZE (0x800)
#define BIOS_CALCULATED_END (BIOS_INTSTACK_BASE + BIOS_INTSTACK_SIZE)

#define SYSCALL_EXITTHREAD 0x666
#define SYSCALL_RETURNFROMEXCEPTION 0x667
#define SYSCALL_RESCHEDULE 0x668
#define SYSCALL_SLEEPTHREAD 0x669
#define SYSCALL_PROCESSMODULESTART 0x66A
#define SYSCALL_FINISHMODULESTART 0x66B
#define SYSCALL_DELAYTHREADTICKS 0x66C

//This is the space needed to preserve at most four arguments in the stack frame (as per MIPS calling convention)
#define STACK_FRAME_RESERVE_SIZE 0x10

#define MODULE_ID_CDVD_EE_DRIVER 0x70000000

CIopBios::CIopBios(CMIPS& cpu, uint8* ram, uint8* spr)
    : m_cpu(cpu)
    , m_ram(ram)
    , m_spr(spr)
    , m_threadFinishAddress(0)
    , m_returnFromExceptionAddress(0)
    , m_idleFunctionAddress(0)
    , m_moduleStarterProcAddress(0)
    , m_alarmThreadProcAddress(0)
    , m_vblankHandlerAddress(0)
    , m_threads(reinterpret_cast<THREAD*>(&m_ram[BIOS_THREADS_BASE]), 1, MAX_THREAD)
    , m_memoryBlocks(reinterpret_cast<Iop::MEMORYBLOCK*>(&ram[BIOS_MEMORYBLOCK_BASE]), 1, MAX_MEMORYBLOCK)
    , m_semaphores(reinterpret_cast<SEMAPHORE*>(&m_ram[BIOS_SEMAPHORES_BASE]), 1, MAX_SEMAPHORE)
    , m_eventFlags(reinterpret_cast<EVENTFLAG*>(&m_ram[BIOS_EVENTFLAGS_BASE]), 1, MAX_EVENTFLAG)
    , m_intrHandlers(reinterpret_cast<INTRHANDLER*>(&m_ram[BIOS_INTRHANDLER_BASE]), 1, MAX_INTRHANDLER)
    , m_vblankHandlers(reinterpret_cast<VBLANKHANDLER*>(&m_ram[BIOS_VBLANKHANDLER_BASE]), 1, MAX_VBLANKHANDLER)
    , m_messageBoxes(reinterpret_cast<MESSAGEBOX*>(&m_ram[BIOS_MESSAGEBOX_BASE]), 1, MAX_MESSAGEBOX)
    , m_fpls(reinterpret_cast<FPL*>(&m_ram[BIOS_FPL_BASE]), 1, MAX_FPL)
    , m_vpls(reinterpret_cast<VPL*>(&m_ram[BIOS_VPL_BASE]), 1, MAX_VPL)
    , m_loadedModules(reinterpret_cast<LOADEDMODULE*>(&m_ram[BIOS_LOADEDMODULE_BASE]), 1, MAX_LOADEDMODULE)
    , m_currentThreadId(reinterpret_cast<uint32*>(m_ram + BIOS_CURRENT_THREAD_ID_BASE))
{
	static_assert(BIOS_CALCULATED_END <= CIopBios::CONTROL_BLOCK_END, "Control block size is too small");
	static_assert(BIOS_SYSTEM_INTRHANDLER_TABLE_BASE > CIopBios::CONTROL_BLOCK_START, "Intr handler table is outside reserved block");
}

CIopBios::~CIopBios()
{
	DeleteModules();
}

void CIopBios::Reset(uint32 ramSize, const Iop::SifManPtr& sifMan)
{
	m_ramSize = ramSize;

	SetDefaultImageVersion(DEFAULT_IMAGE_VERSION);
	PopulateSystemIntcHandlers();

	//Assemble handlers
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[BIOS_HANDLERS_BASE]));
		m_threadFinishAddress = AssembleThreadFinish(assembler);
		m_returnFromExceptionAddress = AssembleReturnFromException(assembler);
		m_idleFunctionAddress = AssembleIdleFunction(assembler);
		m_moduleStarterProcAddress = AssembleModuleStarterProc(assembler);
		m_alarmThreadProcAddress = AssembleAlarmThreadProc(assembler);
		m_vblankHandlerAddress = AssembleVblankHandler(assembler);
		assert(BIOS_HANDLERS_END > ((assembler.GetProgramSize() * 4) + BIOS_HANDLERS_BASE));
	}

	//0xBE00000 = Stupid constant to make FFX PSF happy
	CurrentTime() = 0xBE00000;
	ThreadLinkHead() = 0;
	m_currentThreadId = -1;

	m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_IE;

	m_threads.FreeAll();
	m_semaphores.FreeAll();
	m_intrHandlers.FreeAll();
#ifdef DEBUGGER_INCLUDED
	m_moduleTags.clear();
#endif

	DeleteModules();

	if(!sifMan)
	{
		m_sifMan = std::make_shared<Iop::CSifManNull>();
	}
	else
	{
		m_sifMan = sifMan;
	}

	//Register built-in modules
	{
		m_ioman = std::make_shared<Iop::CIoman>(*this, m_ram);
		RegisterModule(m_ioman);
	}
	{
		m_stdio = std::make_shared<Iop::CStdio>(m_ram, *m_ioman);
		RegisterModule(m_stdio);
	}
	{
		m_sysmem = std::make_shared<Iop::CSysmem>(m_ram, CONTROL_BLOCK_END, m_ramSize, m_memoryBlocks, *m_stdio, *m_ioman, *m_sifMan);
		RegisterModule(m_sysmem);
	}
	{
		m_modload = std::make_shared<Iop::CModload>(*this, m_ram);
		RegisterModule(m_modload);
	}
	{
		RegisterModule(std::make_shared<Iop::CSysclib>(m_ram, m_spr, *m_stdio));
	}
	{
		m_loadcore = std::make_shared<Iop::CLoadcore>(*this, m_ram, *m_sifMan);
		RegisterModule(m_loadcore);
	}
	{
		m_libsd = std::make_shared<Iop::CLibSd>();
	}
	{
		RegisterModule(std::make_shared<Iop::CThbase>(*this, m_ram));
	}
	{
		RegisterModule(std::make_shared<Iop::CThmsgbx>(*this, m_ram));
	}
	{
		RegisterModule(std::make_shared<Iop::CThsema>(*this, m_ram));
	}
	{
		RegisterModule(std::make_shared<Iop::CThfpool>(*this));
	}
	{
		RegisterModule(std::make_shared<Iop::CThvpool>(*this));
	}
	{
		RegisterModule(std::make_shared<Iop::CThevent>(*this, m_ram));
	}
	{
		RegisterModule(std::make_shared<Iop::CHeaplib>(*m_sysmem));
	}
	{
		RegisterModule(std::make_shared<Iop::CTimrman>(*this));
	}
	{
		RegisterModule(std::make_shared<Iop::CIntrman>(*this, m_ram));
	}
	{
		RegisterModule(std::make_shared<Iop::CDmacman>());
	}
	{
		RegisterModule(std::make_shared<Iop::CSecrman>());
	}
	{
		RegisterModule(std::make_shared<Iop::CVblank>(*this));
	}
	{
		m_cdvdman = std::make_shared<Iop::CCdvdman>(*this, m_ram);
		RegisterModule(m_cdvdman);
	}
	{
		RegisterModule(m_sifMan);
	}
	{
		m_sifCmd = std::make_shared<Iop::CSifCmd>(*this, *m_sifMan, *m_sysmem, m_ram);
		RegisterModule(m_sifCmd);
	}
#ifdef _IOP_EMULATE_MODULES
	{
		m_fileIo = std::make_shared<Iop::CFileIo>(*this, m_ram, *m_sifMan, *m_ioman);
		RegisterModule(m_fileIo);
	}
	{
		m_cdvdfsv = std::make_shared<Iop::CCdvdfsv>(*m_sifMan, *m_cdvdman, m_ram);
		RegisterModule(m_cdvdfsv);
	}
	{
		m_mcserv = std::make_shared<Iop::CMcServ>(*this, *m_sifMan, *m_sifCmd, *m_sysmem, m_ram);
		RegisterModule(m_mcserv);
	}
	{
		m_powerOff = std::make_shared<Iop::CPowerOff>(*m_sifMan);
		RegisterModule(m_powerOff);
	}
	{
		m_usbd = std::make_shared<Iop::CUsbd>(*this, m_ram);
		RegisterModule(m_usbd);
	}
	RegisterModule(std::make_shared<Iop::CIomanX>(*m_ioman));
	//RegisterModule(std::make_shared<Iop::CNaplink>(*m_sifMan, *m_ioman));
	{
		m_padman = std::make_shared<Iop::CPadMan>();
		m_mtapman = std::make_shared<Iop::CMtapMan>();
	}

	m_hleModules.insert(std::make_pair("rom0:SIO2MAN", m_padman));
	m_hleModules.insert(std::make_pair("rom0:PADMAN", m_padman));
	m_hleModules.insert(std::make_pair("rom0:XSIO2MAN", m_padman));
	m_hleModules.insert(std::make_pair("rom0:XPADMAN", m_padman));
	m_hleModules.insert(std::make_pair("rom0:XMTAPMAN", m_mtapman));
	m_hleModules.insert(std::make_pair("rom0:MCMAN", m_mcserv));
	m_hleModules.insert(std::make_pair("rom0:MCMANO", m_mcserv));
	m_hleModules.insert(std::make_pair("rom0:MCSERV", m_mcserv));
	m_hleModules.insert(std::make_pair("rom0:XMCMAN", m_mcserv));
	m_hleModules.insert(std::make_pair("rom0:XMCSERV", m_mcserv));
	m_hleModules.insert(std::make_pair("rom0:CDVDMAN", m_cdvdman));
	m_hleModules.insert(std::make_pair("rom0:CDVDFSV", m_cdvdfsv));

#endif

	{
		const int sifDmaBufferSize = 0x1000;
		uint32 sifDmaBufferPtr = m_sysmem->AllocateMemory(sifDmaBufferSize, 0, 0);
		m_sifMan->SetDmaBuffer(sifDmaBufferPtr, sifDmaBufferSize);
	}

	{
		const int sifCmdBufferSize = 0x100;
		uint32 sifCmdBufferPtr = m_sysmem->AllocateMemory(sifCmdBufferSize, 0, 0);
		m_sifMan->SetCmdBuffer(sifCmdBufferPtr, sifCmdBufferSize);
	}

	m_sifMan->PrepareModuleData(m_ram, *m_sysmem);

	InitializeModuleStarter();

	Reschedule();
}

uint32& CIopBios::ThreadLinkHead() const
{
	return *reinterpret_cast<uint32*>(m_ram + BIOS_THREAD_LINK_HEAD_BASE);
}

uint64& CIopBios::CurrentTime() const
{
	return *reinterpret_cast<uint64*>(m_ram + BIOS_CURRENT_TIME_BASE);
}

uint32& CIopBios::ModuleStartRequestHead() const
{
	return *reinterpret_cast<uint32*>(m_ram + BIOS_MODULESTARTREQUEST_HEAD_BASE);
}

uint32& CIopBios::ModuleStartRequestFree() const
{
	return *reinterpret_cast<uint32*>(m_ram + BIOS_MODULESTARTREQUEST_FREE_BASE);
}

void CIopBios::PreLoadState()
{
	//Remove all dynamic modules
	for(auto modulePairIterator = m_modules.begin();
	    modulePairIterator != m_modules.end();)
	{
		if(dynamic_cast<Iop::CDynamic*>(modulePairIterator->second.get()) != nullptr)
		{
			modulePairIterator = m_modules.erase(modulePairIterator);
		}
		else
		{
			modulePairIterator++;
		}
	}
	m_sifCmd->ClearServers();
}

void CIopBios::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto modulesFile = std::make_unique<CRegisterStateCollectionFile>(STATE_MODULES);
	{
		for(const auto& modulePair : m_modules)
		{
			if(auto dynamicModule = std::dynamic_pointer_cast<Iop::CDynamic>(modulePair.second))
			{
				CRegisterState moduleState;
				{
					uint32 importTableAddress = reinterpret_cast<const uint8*>(dynamicModule->GetExportTable()) - m_ram;
					moduleState.SetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS, importTableAddress);
				}
				modulesFile->InsertRegisterState(dynamicModule->GetId().c_str(), std::move(moduleState));
			}
		}
	}
	archive.InsertFile(std::move(modulesFile));

	auto builtInModules = GetBuiltInModules();
	for(const auto& module : builtInModules)
	{
		module->SaveState(archive);
	}

	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_MODULESTARTREQUESTS, m_moduleStartRequests, sizeof(m_moduleStartRequests)));
}

void CIopBios::LoadState(Framework::CZipArchiveReader& archive)
{
	auto builtInModules = GetBuiltInModules();
	for(const auto& module : builtInModules)
	{
		module->LoadState(archive);
	}

	CRegisterStateCollectionFile modulesFile(*archive.BeginReadFile(STATE_MODULES));
	{
		for(const auto& moduleStatePair : modulesFile)
		{
			const auto& moduleState(moduleStatePair.second);
			uint32 importTableAddress = moduleState.GetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS);
			auto module = std::make_shared<Iop::CDynamic>(reinterpret_cast<uint32*>(m_ram + importTableAddress));
			FRAMEWORK_MAYBE_UNUSED bool result = RegisterModule(module);
			assert(result);
		}
	}

	archive.BeginReadFile(STATE_MODULESTARTREQUESTS)->Read(m_moduleStartRequests, sizeof(m_moduleStartRequests));

#ifdef _IOP_EMULATE_MODULES
	//Make sure HLE modules are properly registered
	for(const auto& loadedModule : m_loadedModules)
	{
		if(!loadedModule) continue;
		if(loadedModule->state == MODULE_STATE::HLE)
		{
			auto moduleIterator = std::find_if(m_hleModules.begin(), m_hleModules.end(),
			                                   [&](const auto& modulePair) {
				                                   return strcmp(loadedModule->name, modulePair.second->GetId().c_str()) == 0;
			                                   });
			assert(moduleIterator != std::end(m_hleModules));
			if(moduleIterator != std::end(m_hleModules))
			{
				RegisterHleModule(moduleIterator->second);
			}
		}
	}
#endif

#ifdef DEBUGGER_INCLUDED
	m_cpu.m_analysis->Clear();
	for(const auto& moduleTag : m_moduleTags)
	{
		m_cpu.m_analysis->Analyse(moduleTag.begin, moduleTag.end);
	}
#endif
}

bool CIopBios::IsIdle()
{
	return (m_cpu.m_State.nPC == m_idleFunctionAddress);
}

void CIopBios::InitializeModuleStarter()
{
	memset(m_moduleStartRequests, 0, sizeof(m_moduleStartRequests));

	ModuleStartRequestHead() = MODULESTARTREQUEST::INVALID_PTR;
	ModuleStartRequestFree() = 0;

	//Initialize Module Load Request Free List
	for(unsigned int i = 0; i < (MAX_MODULESTARTREQUEST - 1); i++)
	{
		auto moduleStartRequest = &m_moduleStartRequests[i];
		moduleStartRequest->nextPtr = i + 1;
	}

	m_moduleStartRequests[MAX_MODULESTARTREQUEST - 1].nextPtr = MODULESTARTREQUEST::INVALID_PTR;
}

void CIopBios::RequestModuleStart(MODULESTARTREQUEST_SOURCE requestSource, bool stopRequest, uint32 moduleId, const char* path, const char* args, unsigned int argsLength)
{
	uint32 requestPtr = ModuleStartRequestFree();
	assert(requestPtr != MODULESTARTREQUEST::INVALID_PTR);
	if(requestPtr == MODULESTARTREQUEST::INVALID_PTR)
	{
		CLog::GetInstance().Warn(LOGNAME, "Too many modules to be loaded.");
		return;
	}

	auto moduleStartRequest = &m_moduleStartRequests[requestPtr];

	//Unlink from free list and link in active list (at the end)
	{
		ModuleStartRequestFree() = moduleStartRequest->nextPtr;

		uint32* currentPtr = &ModuleStartRequestHead();
		while(*currentPtr != MODULESTARTREQUEST::INVALID_PTR)
		{
			auto currentModuleLoadRequest = &m_moduleStartRequests[*currentPtr];
			currentPtr = &currentModuleLoadRequest->nextPtr;
		}

		*currentPtr = requestPtr;

		moduleStartRequest->nextPtr = MODULESTARTREQUEST::INVALID_PTR;
	}

	int32 requesterThreadId = -1;
	if(requestSource == MODULESTARTREQUEST_SOURCE::LOCAL)
	{
		requesterThreadId = m_currentThreadId;
		SleepThread();
	}

	moduleStartRequest->moduleId = moduleId;
	moduleStartRequest->stopRequest = stopRequest;
	moduleStartRequest->requesterThreadId = requesterThreadId;

	assert((strlen(path) + 1) <= MODULESTARTREQUEST::MAX_PATH_SIZE);
	strncpy(moduleStartRequest->path, path, MODULESTARTREQUEST::MAX_PATH_SIZE);
	moduleStartRequest->path[MODULESTARTREQUEST::MAX_PATH_SIZE - 1] = 0;

	memcpy(moduleStartRequest->args, args, argsLength);
	moduleStartRequest->argsLength = argsLength;

	uint32 moduleStarterThreadId = TriggerCallback(m_moduleStarterProcAddress);
	//Make sure thread runs at proper priority (Burnout 3 changes priority)
	ChangeThreadPriority(moduleStarterThreadId, MODULE_INIT_PRIORITY);
}

void CIopBios::ProcessModuleStart()
{
	static const auto pushToStack =
	    [](uint8* dst, uint32& stackAddress, const uint8* src, uint32 size) {
		    uint32 fixedSize = ((size + 0x3) & ~0x3);
		    uint32 copyAddress = stackAddress - size;
		    stackAddress -= fixedSize;
		    memcpy(dst + copyAddress, src, size);
		    return copyAddress;
	    };

	uint32 requestPtr = ModuleStartRequestHead();
	assert(requestPtr != MODULESTARTREQUEST::INVALID_PTR);
	if(requestPtr == MODULESTARTREQUEST::INVALID_PTR)
	{
		CLog::GetInstance().Warn(LOGNAME, "Asked to load module when none was requested.");
		return;
	}

	auto moduleStartRequest = &m_moduleStartRequests[requestPtr];

	//Unlink from active list and link in free list
	{
		ModuleStartRequestHead() = moduleStartRequest->nextPtr;

		moduleStartRequest->nextPtr = ModuleStartRequestFree();
		ModuleStartRequestFree() = requestPtr;
	}

	auto loadedModule = m_loadedModules[moduleStartRequest->moduleId];
	assert(loadedModule != nullptr);

	//Patch loader thread context with proper info to invoke module entry proc
	if(!moduleStartRequest->stopRequest)
	{
		const char* path = moduleStartRequest->path;
		const char* args = moduleStartRequest->args;
		uint32 argsLength = moduleStartRequest->argsLength;

		typedef std::vector<uint32> ParamListType;
		ParamListType paramList;

		paramList.push_back(pushToStack(
		    m_ram,
		    m_cpu.m_State.nGPR[CMIPS::SP].nV0,
		    reinterpret_cast<const uint8*>(path),
		    static_cast<uint32>(strlen(path)) + 1));
		if(argsLength != 0)
		{
			uint32 stackArgsBase = pushToStack(
			    m_ram,
			    m_cpu.m_State.nGPR[CMIPS::SP].nV0,
			    reinterpret_cast<const uint8*>(args),
			    argsLength);
			unsigned int argsPos = 0;
			while(argsPos < argsLength)
			{
				uint32 argAddress = stackArgsBase + argsPos;
				const char* arg = reinterpret_cast<const char*>(m_ram) + argAddress;
				unsigned int argLength = static_cast<unsigned int>(strlen(arg)) + 1;
				argsPos += argLength;
				paramList.push_back(argAddress);
			}
		}
		m_cpu.m_State.nGPR[CMIPS::A0].nV0 = static_cast<uint32>(paramList.size());
		//Push an additional null parameter. This is needed by Chessmaster who reads
		//out of the argv bounds (reads from argv[argc])
		paramList.push_back(0);
		for(auto param = paramList.rbegin(); paramList.rend() != param; param++)
		{
			m_cpu.m_State.nGPR[CMIPS::A1].nV0 = pushToStack(
			    m_ram,
			    m_cpu.m_State.nGPR[CMIPS::SP].nV0,
			    reinterpret_cast<const uint8*>(&(*param)),
			    4);
		}
	}
	else
	{
		//When invoking entry routine for module stop, A0 needs to be -1
		m_cpu.m_State.nGPR[CMIPS::A0].nD0 = static_cast<int32>(-1);
	}

	//Allocate stack space for spilling params
	m_cpu.m_State.nGPR[CMIPS::SP].nV0 -= STACK_FRAME_RESERVE_SIZE;

	m_cpu.m_State.nGPR[CMIPS::S0].nV0 = moduleStartRequest->moduleId;
	m_cpu.m_State.nGPR[CMIPS::S1].nV0 = moduleStartRequest->stopRequest;
	m_cpu.m_State.nGPR[CMIPS::S2].nV0 = moduleStartRequest->requesterThreadId;
	m_cpu.m_State.nGPR[CMIPS::GP].nV0 = loadedModule->gp;
	m_cpu.m_State.nGPR[CMIPS::RA].nV0 = m_cpu.m_State.nPC;
	m_cpu.m_State.nPC = loadedModule->entryPoint;
}

void CIopBios::FinishModuleStart()
{
	uint32 moduleId = m_cpu.m_State.nGPR[CMIPS::S0].nV0;
	uint32 stopRequest = m_cpu.m_State.nGPR[CMIPS::S1].nV0;
	int32 requesterThreadId = static_cast<int32>(m_cpu.m_State.nGPR[CMIPS::S2].nV0);
	auto moduleResidentState = static_cast<MODULE_RESIDENT_STATE>(m_cpu.m_State.nGPR[CMIPS::A0].nV0 & 0x3);

	auto loadedModule = m_loadedModules[moduleId];
	assert(loadedModule != nullptr);

	if(!stopRequest)
	{
		assert(loadedModule->state == MODULE_STATE::STOPPED);
		loadedModule->state = MODULE_STATE::STARTED;
		loadedModule->residentState = moduleResidentState;

		OnModuleStarted(moduleId);
	}
	else
	{
		assert(moduleResidentState == MODULE_RESIDENT_STATE::NO_RESIDENT_END);
		assert(loadedModule->state == MODULE_STATE::STARTED);
		loadedModule->state = MODULE_STATE::STOPPED;
	}

	//Make sure interrupts are enabled at the end of this
	//some games disable interrupts but never enable them back! (The Mark of Kri)
	m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_IE;

	if(requesterThreadId == -1)
	{
		//If requesterthreadId is -1, we assume that it came from LOADCORE SIF module
		//We need to notify the EE that the load request is over
		m_sifMan->SendCallReply(Iop::CLoadcore::MODULE_ID, nullptr);
	}
	else
	{
		WakeupThread(requesterThreadId, false);
	}

	//Finish our thread
	ExitThread();
}

int32 CIopBios::LoadModuleFromPath(const char* path, uint32 loadAddress, bool ownsMemory)
{
#ifdef _IOP_EMULATE_MODULES
	//This is needed by some homebrew (ie.: doom.elf) that load modules from BIOS
	auto hleModuleIterator = m_hleModules.find(path);
	if(hleModuleIterator != std::end(m_hleModules))
	{
		return LoadHleModule(hleModuleIterator->second);
	}
#endif
	uint32 handle = m_ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, path);
	if(handle & 0x80000000)
	{
		CLog::GetInstance().Warn(LOGNAME, "Tried to load '%s' which couldn't be found.\r\n", path);
		return -1;
	}
	Iop::Ioman::CScopedFile file(handle, *m_ioman);
	auto stream = m_ioman->GetFileStream(file);
	CElf32File module(*stream);
	return LoadModule(module, path, loadAddress, ownsMemory);
}

int32 CIopBios::LoadModuleFromAddress(uint32 modulePtr, uint32 loadAddress, bool ownsMemory)
{
	CELF32 module(m_ram + modulePtr);
	return LoadModule(module, "", loadAddress, ownsMemory);
}

int32 CIopBios::LoadModuleFromHost(uint8* modulePtr)
{
	CELF32 module(modulePtr);
	return LoadModule(module, "", ~0U, true);
}

int32 CIopBios::LoadModule(CELF32& elf, const char* path, uint32 loadAddress, bool ownsMemory)
{
	//Find .iopmod section
	const auto& header = elf.GetHeader();
	const IOPMOD* iopMod = NULL;
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
		auto sectionHeader = elf.GetSection(i);
		if(sectionHeader->nType != IOPMOD_SECTION_ID) continue;
		iopMod = reinterpret_cast<const IOPMOD*>(elf.GetSectionData(i));
	}

	std::string moduleName = iopMod ? iopMod->moduleName : "";
	if(moduleName.empty())
	{
		moduleName = path;
	}

#ifdef _IOP_EMULATE_MODULES
	auto hleModuleIterator = m_hleModules.find(moduleName);
	if(hleModuleIterator != std::end(m_hleModules))
	{
		return LoadHleModule(hleModuleIterator->second);
	}
#endif

	uint32 loadedModuleId = m_loadedModules.Allocate();
	assert(loadedModuleId != -1);
	if(loadedModuleId == -1) return -1;

	auto loadedModule = m_loadedModules[loadedModuleId];
	loadedModule->ownsMemory = ownsMemory;

	ExecutableRange moduleRange;
	uint32 entryPoint = LoadExecutable(elf, moduleRange, loadAddress);

	assert(iopMod);
	if(iopMod != nullptr)
	{
		//Clear BSS section
		FRAMEWORK_MAYBE_UNUSED uint32 dataSectPos = iopMod->textSectionSize;
		uint32 bssSectPos = iopMod->textSectionSize + iopMod->dataSectionSize;
		uint32 bssSectSize = iopMod->bssSectionSize;
		uint32 moduleSize = moduleRange.second - moduleRange.first;
		if(bssSectSize == 0)
		{
			//Some games (Kya: Dark Lineage) seem to not report a proper value for bss size.
			//Try to compute its value so we can properly clear the section.
			assert(moduleSize >= bssSectPos);
			bssSectSize = moduleSize - bssSectPos;
		}
		else
		{
			//Just make sure that everything checks out
			FRAMEWORK_MAYBE_UNUSED uint32 totalSize = bssSectPos + bssSectSize;
			assert(totalSize == moduleSize);
		}
		memset(m_ram + moduleRange.first + bssSectPos, 0, bssSectSize);
	}

	//Fill in module info
	strncpy(loadedModule->name, moduleName.c_str(), LOADEDMODULE::MAX_NAME_SIZE);
	loadedModule->version = iopMod->moduleVersion;
	loadedModule->start = moduleRange.first;
	loadedModule->end = moduleRange.second;
	loadedModule->entryPoint = entryPoint;
	loadedModule->gp = iopMod ? (iopMod->gp + moduleRange.first) : 0;
	loadedModule->state = MODULE_STATE::STOPPED;

#ifdef DEBUGGER_INCLUDED
	PrepareModuleDebugInfo(elf, moduleRange, moduleName, path);
#endif

	OnModuleLoaded(loadedModule->name);

	//Patch for Shadow Hearts PSF set --------------------------------
	if(strstr(path, "RSSD_patchmore.IRX") != NULL)
	{
		const uint32 patchAddress = moduleRange.first + 0xCE0;
		uint32 instruction = m_cpu.m_pMemoryMap->GetWord(patchAddress);
		if(instruction == 0x1200FFFB)
		{
			m_cpu.m_pMemoryMap->SetWord(patchAddress, 0x1000FFFB);
		}
	}

	return loadedModuleId;
}

int32 CIopBios::UnloadModule(uint32 loadedModuleId)
{
	if(loadedModuleId == MODULE_ID_CDVD_EE_DRIVER)
	{
		return 0;
	}

	auto loadedModule = m_loadedModules[loadedModuleId];
	if(loadedModule == nullptr)
	{
		CLog::GetInstance().Warn(LOGNAME, "UnloadModule failed because specified module (%d) doesn't exist.\r\n",
		                         loadedModuleId);
		return -1;
	}
	if(loadedModule->state != MODULE_STATE::STOPPED)
	{
		CLog::GetInstance().Warn(LOGNAME, "UnloadModule failed because specified module (%d) wasn't stopped.\r\n",
		                         loadedModuleId);
		return -1;
	}

	//TODO: Remove module from IOP module list?
	//TODO: Invalidate MIPS analysis range?
	m_cpu.m_executor->ClearActiveBlocksInRange(loadedModule->start, loadedModule->end, false);

	if(loadedModule->ownsMemory)
	{
		//TODO: Check return value here.
		m_sysmem->FreeMemory(loadedModule->start);
	}
	m_loadedModules.Free(loadedModuleId);
	return loadedModuleId;
}

int32 CIopBios::StartModule(MODULESTARTREQUEST_SOURCE requestSource, uint32 loadedModuleId, const char* path, const char* args, uint32 argsLength)
{
	auto loadedModule = m_loadedModules[loadedModuleId];
	if(loadedModule == nullptr)
	{
		return -1;
	}
	if(loadedModule->state == MODULE_STATE::HLE)
	{
		//HLE modules don't need to be started
		return loadedModuleId;
	}
	assert(loadedModule->state == MODULE_STATE::STOPPED);
	RequestModuleStart(requestSource, false, loadedModuleId, path, args, argsLength);
	return loadedModuleId;
}

int32 CIopBios::StopModule(MODULESTARTREQUEST_SOURCE requestSource, uint32 loadedModuleId)
{
	auto loadedModule = m_loadedModules[loadedModuleId];
	if(loadedModule == nullptr)
	{
		CLog::GetInstance().Warn(LOGNAME, "StopModule failed because specified module (%d) doesn't exist.\r\n",
		                         loadedModuleId);
		return -1;
	}
	if(loadedModule->state != MODULE_STATE::STARTED)
	{
		CLog::GetInstance().Warn(LOGNAME, "StopModule failed because specified module (%d) wasn't started.\r\n",
		                         loadedModuleId);
		return -1;
	}
	if(loadedModule->residentState != MODULE_RESIDENT_STATE::REMOVABLE_RESIDENT_END)
	{
		CLog::GetInstance().Warn(LOGNAME, "StopModule failed because specified module (%d) isn't removable.\r\n",
		                         loadedModuleId);
		return -1;
	}
	RequestModuleStart(requestSource, true, loadedModuleId, "other", nullptr, 0);
	return loadedModuleId;
}

bool CIopBios::CanStopModule(uint32 loadedModuleId) const
{
	return (loadedModuleId != MODULE_ID_CDVD_EE_DRIVER);
}

bool CIopBios::IsModuleHle(uint32 loadedModuleId) const
{
	auto loadedModule = m_loadedModules[loadedModuleId];
	if(!loadedModule)
	{
		return false;
	}
	return (loadedModule->state == MODULE_STATE::HLE);
}

int32 CIopBios::SearchModuleByName(const char* moduleName) const
{
	for(unsigned int i = 0; i < MAX_LOADEDMODULE; i++)
	{
		auto loadedModule = m_loadedModules[i];
		if(loadedModule == nullptr) continue;
		if(!strcmp(loadedModule->name, moduleName))
		{
			return i;
		}
	}
	if(!strcmp(moduleName, "cdvd_ee_driver"))
	{
		return MODULE_ID_CDVD_EE_DRIVER;
	}
	return KERNEL_RESULT_ERROR_UNKNOWN_MODULE;
}

int32 CIopBios::ReferModuleStatus(uint32 moduleId, uint32 statusPtr)
{
	auto loadedModule = m_loadedModules[moduleId];
	if(loadedModule == nullptr)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_MODULE;
	}

	auto moduleStatus = reinterpret_cast<MODULE_INFO*>(m_ram + statusPtr);
	strncpy(moduleStatus->name, loadedModule->name, MODULE_INFO::MAX_NAME_SIZE);
	moduleStatus->version = loadedModule->version;
	moduleStatus->id = moduleId;
	return KERNEL_RESULT_OK;
}

void CIopBios::ProcessModuleReset(const std::string& initCommand)
{
	CLog::GetInstance().Print(LOGNAME, "ProcessModuleReset(%s);\r\n", initCommand.c_str());

	UnloadUserComponents();

	uint32 imageVersion = m_defaultImageVersion;

	auto initArguments = StringUtils::Split(initCommand);
	assert(initArguments.size() >= 1);
	if(initArguments.size() >= 1)
	{
		auto loaderPath = initArguments[0];
		assert(loaderPath.find("UDNL") != std::string::npos);
		if(initArguments.size() >= 2)
		{
			auto imagePath = initArguments[1];
			bool found = false;
			found = TryGetImageVersionFromContents(imagePath, &imageVersion);
			if(!found) found = TryGetImageVersionFromPath(imagePath, &imageVersion);
			if(!found)
			{
				CLog::GetInstance().Warn(LOGNAME, "Failed to find IOP module version from '%s'.\r\n", imagePath.c_str());
			}
		}
	}

	m_loadcore->SetModuleVersion(imageVersion);
#ifdef _IOP_EMULATE_MODULES
	m_fileIo->SetModuleVersion(imageVersion);
	m_mcserv->SetModuleVersion(imageVersion);
#endif
}

void CIopBios::SetDefaultImageVersion(uint32 defaultImageVersion)
{
	m_defaultImageVersion = defaultImageVersion;
}

bool CIopBios::TryGetImageVersionFromPath(const std::string& imagePath, unsigned int* result)
{
	struct IMAGE_FILE_PATTERN
	{
		const char* start;
		const char* pattern;
	};
	// clang-format off
	static const IMAGE_FILE_PATTERN g_imageFilePatterns[] =
	{
		{"IOPRP", "IOPRP%d.IMG;1"},
		{"DNAS", "DNAS%d.IMG;1"}
	};
	// clang-format on

	for(const auto imageFilePattern : g_imageFilePatterns)
	{
		auto imageFileName = strstr(imagePath.c_str(), imageFilePattern.start);
		if(imageFileName != nullptr)
		{
			unsigned int imageVersion = 0;
			auto cvtCount = sscanf(imageFileName, imageFilePattern.pattern, &imageVersion);
			if(cvtCount == 1)
			{
				if(imageVersion < 100)
				{
					imageVersion = imageVersion * 100;
				}
				else
				{
					imageVersion = imageVersion * 10;
				}
				if(result)
				{
					(*result) = imageVersion;
				}
				return true;
			}
		}
	}
	return false;
}

bool CIopBios::TryGetImageVersionFromContents(const std::string& imagePath, unsigned int* result)
{
	int32 fd = m_ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, imagePath.c_str());
	if(fd < 0) return false;

	//Some notes about this:
	//- FFXI/POLViewer resets the IOP with a image without version in the filename
	//  and it also doesn't have a fileio string that lets us find the required version,
	//  thus, we rely on the ioprp pattern for this game.
	static const std::string fileIoPatternString = "PsIIfileio  ";
	static const std::string sysmemPatternString = "PsIIsysmem  ";
	static const std::string loadcorePatternString = "PsIIloadcore";
	static const std::string ioprpPatternString = "ioprp";

	auto tryMatchVersionPattern =
	    [result](auto moduleVersionString, const std::string& patternString) {
		    if(!strncmp(moduleVersionString, patternString.c_str(), patternString.size()))
		    {
			    //Found something
			    unsigned int imageVersion = atoi(moduleVersionString + patternString.size());
			    if(imageVersion < 1000) return false;
			    if(result)
			    {
				    (*result) = imageVersion;
			    }
			    return true;
		    }
		    else
		    {
			    return false;
		    }
	    };

	Iop::Ioman::CScopedFile file(fd, *m_ioman);
	auto stream = m_ioman->GetFileStream(file);
	while(1)
	{
		static const unsigned int moduleVersionStringSize = 0x10;
		char moduleVersionString[moduleVersionStringSize + 1];
		auto currentPos = stream->Tell();
		auto readAmount = stream->Read(moduleVersionString, moduleVersionStringSize);
		if(readAmount != moduleVersionStringSize)
		{
			//We've hit the end
			break;
		}
		moduleVersionString[moduleVersionStringSize] = 0;
		if(tryMatchVersionPattern(moduleVersionString, fileIoPatternString))
		{
			return true;
		}
		else if(tryMatchVersionPattern(moduleVersionString, sysmemPatternString))
		{
			return true;
		}
		else if(tryMatchVersionPattern(moduleVersionString, loadcorePatternString))
		{
			return true;
		}
		else if(tryMatchVersionPattern(moduleVersionString, ioprpPatternString))
		{
			return true;
		}
		stream->Seek(currentPos + 1, Framework::STREAM_SEEK_SET);
	}
	return false;
}

CIopBios::THREAD* CIopBios::GetThread(uint32 threadId)
{
	return m_threads[threadId];
}

int32 CIopBios::GetCurrentThreadId()
{
	int32 threadId = m_currentThreadId;
	if(threadId < 0)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_CONTEXT;
	}
	else
	{
		return threadId;
	}
}

int32 CIopBios::GetCurrentThreadIdRaw() const
{
	return m_currentThreadId;
}

uint32 CIopBios::CreateThread(uint32 threadProc, uint32 priority, uint32 stackSize, uint32 optionData, uint32 attributes)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: CreateThread(threadProc = 0x%08X, priority = %d, stackSize = 0x%08X, attributes = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), threadProc, priority, stackSize, attributes);
#endif

	//Thread proc address needs to be 4-bytes aligned
	if((threadProc & 0x3) != 0)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_ENTRY;
	}

	//Priority needs to be between [1, 126]
	if((priority < 1) || (priority > 126))
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_PRIORITY;
	}

	if(stackSize == 0)
	{
		stackSize = DEFAULT_STACKSIZE;
	}

	//Make sure stack size is a multiple of 4
	stackSize = (stackSize + 0x03) & ~0x03;

	uint32 stackBase = m_sysmem->AllocateMemory(stackSize, 0, 0);
	if(stackBase == 0)
	{
		return KERNEL_RESULT_ERROR_NO_MEMORY;
	}

	uint32 threadId = m_threads.Allocate();
	assert(threadId != -1);
	if(threadId == -1)
	{
		m_sysmem->FreeMemory(stackBase);
		return -1;
	}

	auto thread = m_threads[threadId];
	thread->context = {};
	thread->context.delayJump = MIPS_INVALID_PC;
	thread->stackSize = stackSize;
	thread->stackBase = stackBase;
	memset(m_ram + thread->stackBase, 0xFF, thread->stackSize);
	thread->id = threadId;
	thread->priority = 0;
	thread->initPriority = priority;
	thread->status = THREAD_STATUS_DORMANT;
	thread->threadProc = threadProc;
	thread->optionData = optionData;
	thread->attributes = attributes;
	thread->nextActivateTime = 0;
	thread->wakeupCount = 0;
	thread->context.gpr[CMIPS::GP] = m_cpu.m_State.nGPR[CMIPS::GP].nV0;
	thread->context.gpr[CMIPS::SP] = thread->stackBase + thread->stackSize - STACK_FRAME_RESERVE_SIZE;
	return thread->id;
}

int32 CIopBios::DeleteThread(uint32 threadId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: DeleteThread(threadId = %d);\r\n",
	                          m_currentThreadId.Get(), threadId);
#endif

	if(threadId == 0)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_THID;
	}

	auto thread = m_threads[threadId];
	if(!thread)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_THID;
	}

	if(thread->status != THREAD_STATUS_DORMANT)
	{
		return KERNEL_RESULT_ERROR_NOT_DORMANT;
	}

	UnlinkThread(threadId);
	m_sysmem->FreeMemory(thread->stackBase);
	m_threads.Free(threadId);

	return KERNEL_RESULT_OK;
}

int32 CIopBios::StartThread(uint32 threadId, uint32 param)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: StartThread(threadId = %i, param = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), threadId, param);
#endif

	auto thread = GetThread(threadId);
	assert(thread != nullptr);
	if(thread == nullptr)
	{
		return -1;
	}

	if(thread->status != THREAD_STATUS_DORMANT)
	{
		CLog::GetInstance().Warn(LOGNAME, "%d: Failed to start thread %d, thread not dormant.\r\n",
		                         m_currentThreadId.Get(), threadId);
		assert(false);
		return -1;
	}

	thread->status = THREAD_STATUS_RUNNING;
	thread->priority = thread->initPriority;
	LinkThread(threadId);
	thread->context.epc = thread->threadProc;
	thread->context.gpr[CMIPS::A0] = param;
	thread->context.gpr[CMIPS::RA] = m_threadFinishAddress;
	thread->context.gpr[CMIPS::SP] = thread->stackBase + thread->stackSize - STACK_FRAME_RESERVE_SIZE;

	//IOP BIOS disassembly shows that this is done at thread start.
	//Not sure why it's needed, but some games seem to rely on it (Armored Core 2: Another Age)
	//because thread stack is initialized to 0xFF but it needs a part of it to be set to 0
	{
		uint32 alignedStackSize = thread->stackSize & ~0x3;
		uint32 stackTopClearSize = std::min<uint32>(alignedStackSize, 0xB8);
		uint32 clearArea = thread->stackBase + alignedStackSize - stackTopClearSize;
		memset(m_ram + clearArea, 0, stackTopClearSize);
	}

	m_rescheduleNeeded = true;
	return 0;
}

int32 CIopBios::StartThreadArgs(uint32 threadId, uint32 args, uint32 argpPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: StartThreadArgs(threadId = %d, args = %d, argp = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), threadId, args, argpPtr);
#endif

	auto thread = GetThread(threadId);
	assert(thread != nullptr);
	if(thread == nullptr)
	{
		return -1;
	}

	if(thread->status != THREAD_STATUS_DORMANT)
	{
		CLog::GetInstance().Warn(LOGNAME, "%d: Failed to start thread %d, thread not dormant.\r\n",
		                         m_currentThreadId.Get(), threadId);
		assert(false);
		return -1;
	}

	static const auto pushToStack =
	    [](uint8* dst, uint32& stackAddress, const uint8* src, uint32 size) {
		    uint32 fixedSize = ((size + 0x3) & ~0x3);
		    uint32 copyAddress = stackAddress - fixedSize;
		    stackAddress -= fixedSize;
		    memcpy(dst + copyAddress, src, size);
		    return copyAddress;
	    };

	thread->status = THREAD_STATUS_RUNNING;
	LinkThread(threadId);
	thread->priority = thread->initPriority;
	thread->context.epc = thread->threadProc;
	thread->context.gpr[CMIPS::RA] = m_threadFinishAddress;
	thread->context.gpr[CMIPS::SP] = thread->stackBase + thread->stackSize;

	thread->context.gpr[CMIPS::A0] = args;
	thread->context.gpr[CMIPS::A1] = pushToStack(m_ram, thread->context.gpr[CMIPS::SP], m_ram + argpPtr, args);

	thread->context.gpr[CMIPS::SP] -= STACK_FRAME_RESERVE_SIZE;

	m_rescheduleNeeded = true;
	return 0;
}

void CIopBios::ExitThread()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: ExitThread();\r\n", m_currentThreadId.Get());
#endif
	THREAD* thread = GetThread(m_currentThreadId);
	thread->status = THREAD_STATUS_DORMANT;
	assert(thread->waitSemaphore == 0);
	UnlinkThread(thread->id);
	m_rescheduleNeeded = true;
}

uint32 CIopBios::TerminateThread(uint32 threadId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: TerminateThread(threadId = %d);\r\n",
	                          m_currentThreadId.Get(), threadId);
#endif

	assert(threadId != m_currentThreadId);
	if(threadId == m_currentThreadId)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_THID;
	}

	auto thread = GetThread(threadId);
	assert(thread != nullptr);
	if(thread == nullptr)
	{
		return -1;
	}
	if(thread->waitSemaphore != 0)
	{
		auto semaphore = m_semaphores[thread->waitSemaphore];
		if(semaphore != nullptr)
		{
			assert(semaphore->waitCount > 0);
			semaphore->waitCount--;
		}
		thread->waitSemaphore = 0;
	}
	thread->status = THREAD_STATUS_DORMANT;
	UnlinkThread(thread->id);
	return KERNEL_RESULT_OK;
}

int32 CIopBios::DelayThread(uint32 delay)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: DelayThread(delay = %i);\r\n",
	                          m_currentThreadId.Get(), delay);
#endif

	THREAD* thread = GetThread(m_currentThreadId);
	thread->nextActivateTime = GetCurrentTime() + MicroSecToClock(delay);
	//TODO: Add a proper wait state to allow thread to be relinked
	//at the end of the queue at the right moment
	UnlinkThread(thread->id);
	LinkThread(thread->id);
	m_rescheduleNeeded = true;

	return KERNEL_RESULT_OK;
}

void CIopBios::DelayThreadTicks(uint32 delay)
{
	auto thread = GetThread(m_currentThreadId);
	thread->nextActivateTime = GetCurrentTime() + delay;
	//TODO: Add a proper wait state to allow thread to be relinked
	//at the end of the queue at the right moment
	UnlinkThread(thread->id);
	LinkThread(thread->id);
	m_rescheduleNeeded = true;
}

uint32 CIopBios::SetAlarm(uint32 timePtr, uint32 alarmFunction, uint32 param)
{
	uint32 alarmThreadId = -1;

	//Find a thread we could recycle for a new alarm
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->threadProc != m_alarmThreadProcAddress) continue;
		if(thread->status == THREAD_STATUS_DORMANT)
		{
			alarmThreadId = thread->id;
			break;
		}
	}

	//If no threads are available, create a new one
	if(alarmThreadId == -1)
	{
		alarmThreadId = CreateThread(m_alarmThreadProcAddress, 1, DEFAULT_STACKSIZE, 0, 0);
	}

	StartThread(alarmThreadId, 0);

	auto thread = GetThread(alarmThreadId);
	thread->context.gpr[CMIPS::SP] -= 0x20;

	uint32* delay = reinterpret_cast<uint32*>(m_ram + timePtr);
	assert(delay[1] == 0);

	*reinterpret_cast<uint32*>(m_ram + thread->context.gpr[CMIPS::SP] + 0x00) = alarmFunction;
	*reinterpret_cast<uint32*>(m_ram + thread->context.gpr[CMIPS::SP] + 0x04) = param;
	*reinterpret_cast<uint32*>(m_ram + thread->context.gpr[CMIPS::SP] + 0x08) = delay[0];

	thread->context.gpr[CMIPS::A0] = thread->context.gpr[CMIPS::SP];
	thread->optionData = alarmFunction;

	//Returns negative value on failure
	return 0;
}

uint32 CIopBios::CancelAlarm(uint32 alarmFunction, uint32 param, bool inInterrupt)
{
	//TODO: This needs to garantee that the alarm handler function won't be called after the cancel

	uint32 alarmThreadId = -1;

	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->status == THREAD_STATUS_DORMANT) continue;
		if(thread->optionData != alarmFunction) continue;
		if(thread->threadProc == m_alarmThreadProcAddress)
		{
			alarmThreadId = thread->id;
			break;
		}
	}

	if(alarmThreadId == -1)
	{
		// handler not registered
		return KERNEL_RESULT_ERROR_NOTFOUND_HANDLER;
	}

	TerminateThread(alarmThreadId);
	return 0;
}

int32 CIopBios::ChangeThreadPriority(uint32 threadId, uint32 newPrio)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ChangeThreadPriority(threadId = %d, newPrio = %d);\r\n",
	                          m_currentThreadId.Get(), threadId, newPrio);
#endif

	//Priority needs to be between [1, 126]
	if((newPrio < 1) || (newPrio > 126))
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_PRIORITY;
	}

	if(threadId == 0)
	{
		threadId = m_currentThreadId;
	}

	auto thread = GetThread(threadId);
	assert(thread);
	if(!thread)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_THID;
	}

	thread->priority = newPrio;
	if(thread->status == THREAD_STATUS_RUNNING)
	{
		UnlinkThread(threadId);
		LinkThread(threadId);
	}

	m_rescheduleNeeded = true;

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::ReferThreadStatus(uint32 threadId, uint32 statusPtr, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReferThreadStatus(threadId = %d, statusPtr = 0x%08X, inInterrupt = %d);\r\n",
	                          m_currentThreadId.Get(), threadId, statusPtr, inInterrupt);
#endif

	if(threadId == 0)
	{
		threadId = m_currentThreadId;
	}

	auto thread = m_threads[threadId];
	assert(thread);
	if(!thread)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_THID;
	}

	uint32 threadStatus = 0;
	switch(thread->status)
	{
	case THREAD_STATUS_DORMANT:
		threadStatus = 0x10;
		break;
	case THREAD_STATUS_SLEEPING:
	case THREAD_STATUS_WAITING_MESSAGEBOX:
	case THREAD_STATUS_WAITING_EVENTFLAG:
	case THREAD_STATUS_WAITING_SEMAPHORE:
	case THREAD_STATUS_WAIT_VBLANK_START:
	case THREAD_STATUS_WAIT_VBLANK_END:
		threadStatus = 0x04;
		break;
	case THREAD_STATUS_RUNNING:
		if(threadId == m_currentThreadId)
		{
			threadStatus = 0x01;
		}
		else
		{
			threadStatus = 0x02;
		}
		break;
	default:
		threadStatus = 0;
		break;
	}

	uint32 waitType = 0;
	switch(thread->status)
	{
	case THREAD_STATUS_SLEEPING:
		waitType = 1;
		break;
	case THREAD_STATUS_WAITING_SEMAPHORE:
		waitType = 3;
		break;
	case THREAD_STATUS_WAITING_EVENTFLAG:
		waitType = 4;
		break;
	case THREAD_STATUS_WAITING_MESSAGEBOX:
		waitType = 5;
		break;
	default:
		waitType = 0;
		break;
	}

	auto threadInfo = reinterpret_cast<THREAD_INFO*>(m_ram + statusPtr);
	threadInfo->option = thread->optionData;
	threadInfo->attributes = thread->attributes;
	threadInfo->status = threadStatus;
	threadInfo->entryPoint = thread->threadProc;
	threadInfo->stackAddr = thread->stackBase;
	threadInfo->stackSize = thread->stackSize;
	threadInfo->initPriority = thread->initPriority;
	threadInfo->currentPriority = thread->priority;
	threadInfo->waitType = waitType;

	return KERNEL_RESULT_OK;
}

int32 CIopBios::SleepThread()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: SleepThread();\r\n",
	                          m_currentThreadId.Get());
#endif

	THREAD* thread = GetThread(m_currentThreadId);
	if(thread->status != THREAD_STATUS_RUNNING)
	{
		throw std::runtime_error("Thread isn't running.");
	}
	if(thread->wakeupCount == 0)
	{
		thread->status = THREAD_STATUS_SLEEPING;
		UnlinkThread(thread->id);
		m_rescheduleNeeded = true;
	}
	else
	{
		thread->wakeupCount--;
	}

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::WakeupThread(uint32 threadId, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: WakeupThread(threadId = %d);\r\n",
	                          m_currentThreadId.Get(), threadId);
#endif

	auto thread = GetThread(threadId);
	if(!thread)
	{
		CLog::GetInstance().Warn(LOGNAME, "%d: Trying to wakeup a non-existing thread (%d).\r\n",
		                         m_currentThreadId.Get(), threadId);
		return KERNEL_RESULT_ERROR_UNKNOWN_THID;
	}

	if(thread->status == THREAD_STATUS_SLEEPING)
	{
		thread->status = THREAD_STATUS_RUNNING;
		LinkThread(threadId);
		if(!inInterrupt)
		{
			m_rescheduleNeeded = true;
		}
	}
	else
	{
		thread->wakeupCount++;
	}
	return thread->wakeupCount;
}

int32 CIopBios::CancelWakeupThread(uint32 threadId, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: CancelWakeupThread(threadId = %d);\r\n",
	                          m_currentThreadId.Get(), threadId);
#endif

	if(threadId == 0)
	{
		threadId = m_currentThreadId;
	}

	auto thread = GetThread(threadId);
	if(!thread)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_THID;
	}

	uint32 result = thread->wakeupCount;
	thread->wakeupCount = 0;

	return result;
}

int32 CIopBios::RotateThreadReadyQueue(uint32 priority)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: RotateThreadReadyQueue(priority = %d);\r\n",
	                          m_currentThreadId.Get(), priority);
#endif

	if(priority == 0)
	{
		auto thread = GetThread(m_currentThreadId);
		priority = thread->priority;
	}

	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
		auto nextThread = m_threads[nextThreadId];
		if(nextThread->priority == priority)
		{
			UnlinkThread(nextThreadId);
			LinkThread(nextThreadId);
			m_rescheduleNeeded = true;
			break;
		}
		nextThreadId = nextThread->nextThreadId;
	}

	return KERNEL_RESULT_OK;
}

int32 CIopBios::ReleaseWaitThread(uint32 threadId, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReleaseWaitThread(threadId = %d);\r\n",
	                          m_currentThreadId.Get(), threadId);
#endif

	if(threadId == 0)
	{
		threadId = m_currentThreadId;
	}

	if(threadId == m_currentThreadId)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_THID;
	}

	auto thread = GetThread(threadId);
	if(!thread)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_THID;
	}

	if(
	    (thread->status == THREAD_STATUS_RUNNING) ||
	    (thread->status == THREAD_STATUS_DORMANT))
	{
		return KERNEL_RESULT_ERROR_NOT_WAIT;
	}

	switch(thread->status)
	{
	case THREAD_STATUS_SLEEPING:
		//Nothing special to do
		break;
	case THREAD_STATUS_WAITING_SEMAPHORE:
	{
		auto semaphore = m_semaphores[thread->waitSemaphore];
		assert(semaphore);
		assert(semaphore->waitCount != 0);
		semaphore->waitCount--;
		thread->waitSemaphore = 0;
	}
	break;
	case THREAD_STATUS_WAITING_EVENTFLAG:
		thread->waitEventFlag = 0;
		thread->waitEventFlagResultPtr = 0;
		break;
	default:
		assert(false);
		break;
	}

	//Update return value for waiting thread
	thread->context.gpr[CMIPS::V0] = KERNEL_RESULT_ERROR_RELEASE_WAIT;

	thread->status = THREAD_STATUS_RUNNING;
	LinkThread(threadId);
	if(!inInterrupt)
	{
		m_rescheduleNeeded = true;
	}

	return KERNEL_RESULT_OK;
}

int32 CIopBios::RegisterVblankHandler(uint32 startEnd, uint32 priority, uint32 handlerPtr, uint32 handlerParam)
{
	assert((startEnd == 0) || (startEnd == 1));

	//Make sure interrupt handler is registered
	{
		uint32 intrLine = startEnd ? Iop::CIntc::LINE_EVBLANK : Iop::CIntc::LINE_VBLANK;
		uint32 intrHandlerId = FindIntrHandler(intrLine);
		if(intrHandlerId == -1)
		{
			RegisterIntrHandler(intrLine, 0, m_vblankHandlerAddress, startEnd);

			uint32 mask = m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::MASK0);
			mask |= (1 << intrLine);
			m_cpu.m_pMemoryMap->SetWord(Iop::CIntc::MASK0, mask);
		}
	}

	// Check if the exact handler is already registered
	if(FindVblankHandlerByLineAndPtr(startEnd, handlerPtr) != -1)
	{
		return KERNEL_RESULT_ERROR_FOUND_HANDLER;
	}

	uint32 handlerId = m_vblankHandlers.Allocate();
	if(handlerId == -1)
	{
		return KERNEL_RESULT_ERROR_NO_MEMORY;
	}

	auto handler = m_vblankHandlers[handlerId];
	assert(handler);
	handler->handler = handlerPtr;
	handler->arg = handlerParam;
	handler->type = startEnd;

	return KERNEL_RESULT_OK;
}

int32 CIopBios::ReleaseVblankHandler(uint32 startEnd, uint32 handlerPtr)
{
	assert((startEnd == 0) || (startEnd == 1));

	uint32 handlerId = FindVblankHandlerByLineAndPtr(startEnd, handlerPtr);
	if(handlerId == -1)
	{
		return KERNEL_RESULT_ERROR_NOTFOUND_HANDLER;
	}

	m_vblankHandlers.Free(handlerId);

	return KERNEL_RESULT_OK;
}

int32 CIopBios::FindVblankHandlerByLineAndPtr(uint32 startEnd, uint32 handlerPtr)
{
	uint32 handlerId = -1;
	for(unsigned int i = 0; i < MAX_VBLANKHANDLER; i++)
	{
		if(m_vblankHandlers[i] != nullptr && m_vblankHandlers[i]->handler == handlerPtr && m_vblankHandlers[i]->type == startEnd)
		{
			handlerId = i;
			break;
		}
	}

	return handlerId;
}

void CIopBios::SleepThreadTillVBlankStart()
{
	THREAD* thread = GetThread(m_currentThreadId);
	thread->status = THREAD_STATUS_WAIT_VBLANK_START;
	UnlinkThread(thread->id);
	m_rescheduleNeeded = true;
}

void CIopBios::SleepThreadTillVBlankEnd()
{
	THREAD* thread = GetThread(m_currentThreadId);
	thread->status = THREAD_STATUS_WAIT_VBLANK_END;
	UnlinkThread(thread->id);
	m_rescheduleNeeded = true;
}

void CIopBios::LoadThreadContext(uint32 threadId)
{
	THREAD* thread = GetThread(threadId);
	for(unsigned int i = 0; i < 32; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		m_cpu.m_State.nGPR[i].nD0 = static_cast<int32>(thread->context.gpr[i]);
	}
	m_cpu.m_State.nLO[0] = thread->context.gpr[CMIPS::K0];
	m_cpu.m_State.nHI[0] = thread->context.gpr[CMIPS::K1];
	m_cpu.m_State.nPC = thread->context.epc;
	m_cpu.m_State.nDelayedJumpAddr = thread->context.delayJump;
}

void CIopBios::SaveThreadContext(uint32 threadId)
{
	THREAD* thread = GetThread(threadId);
	for(unsigned int i = 0; i < 32; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		thread->context.gpr[i] = m_cpu.m_State.nGPR[i].nV0;
	}
	thread->context.gpr[CMIPS::K0] = m_cpu.m_State.nLO[0];
	thread->context.gpr[CMIPS::K1] = m_cpu.m_State.nHI[0];
	thread->context.epc = m_cpu.m_State.nPC;
	thread->context.delayJump = m_cpu.m_State.nDelayedJumpAddr;
}

void CIopBios::LinkThread(uint32 threadId)
{
	auto thread = m_threads[threadId];
	auto nextThreadId = &ThreadLinkHead();
	while(1)
	{
		assert((*nextThreadId) < MAX_THREAD);
		if((*nextThreadId) == 0)
		{
			(*nextThreadId) = threadId;
			thread->nextThreadId = 0;
			break;
		}
		auto currentThread = m_threads[(*nextThreadId)];
		if(currentThread->priority > thread->priority)
		{
			thread->nextThreadId = (*nextThreadId);
			(*nextThreadId) = threadId;
			break;
		}
		nextThreadId = &currentThread->nextThreadId;
	}
}

void CIopBios::UnlinkThread(uint32 threadId)
{
	THREAD* thread = m_threads[threadId];
	uint32* nextThreadId = &ThreadLinkHead();
	while(1)
	{
		if((*nextThreadId) == 0)
		{
			break;
		}
		THREAD* currentThread = m_threads[(*nextThreadId)];
		if((*nextThreadId) == threadId)
		{
			(*nextThreadId) = thread->nextThreadId;
			thread->nextThreadId = 0;
			break;
		}
		nextThreadId = &currentThread->nextThreadId;
	}
}

void CIopBios::Reschedule()
{
	if((m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL) != 0)
	{
		return;
	}

	//Don't switch if interrupts are disabled
	if((m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_IE) != CMIPS::STATUS_IE)
	{
		return;
	}

	if(m_currentThreadId != -1)
	{
		SaveThreadContext(m_currentThreadId);
	}

	uint32 nextThreadId = GetNextReadyThread();
	if(nextThreadId == -1)
	{
		m_cpu.m_State.nPC = m_idleFunctionAddress;
	}
	else
	{
		LoadThreadContext(nextThreadId);
	}
#ifdef _DEBUG
	if(nextThreadId != m_currentThreadId)
	{
		CLog::GetInstance().Print(LOGNAME, "Switched over to thread %i.\r\n", nextThreadId);
	}
#endif
	m_currentThreadId = nextThreadId;
}

uint32 CIopBios::GetNextReadyThread()
{
	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
		THREAD* nextThread = m_threads[nextThreadId];
		nextThreadId = nextThread->nextThreadId;
		if(GetCurrentTime() <= nextThread->nextActivateTime) continue;
		assert(nextThread->status == THREAD_STATUS_RUNNING);
		return nextThread->id;
	}
	return -1;
}

uint64 CIopBios::GetCurrentTime() const
{
	return CurrentTime();
}

uint64 CIopBios::MilliSecToClock(uint32 value)
{
	return (static_cast<uint64>(value) * static_cast<uint64>(PS2::IOP_CLOCK_OVER_FREQ)) / 1000;
}

uint64 CIopBios::MicroSecToClock(uint32 value)
{
	return (static_cast<uint64>(value) * static_cast<uint64>(PS2::IOP_CLOCK_OVER_FREQ)) / 1000000;
}

uint64 CIopBios::ClockToMicroSec(uint64 clock)
{
	return (clock * 1000000) / static_cast<uint64>(PS2::IOP_CLOCK_OVER_FREQ);
}

void CIopBios::CountTicks(uint32 ticks)
{
	CurrentTime() += ticks;
	m_sifMan->CountTicks(ticks);
#ifdef _IOP_EMULATE_MODULES
	m_cdvdman->CountTicks(ticks);
	m_mcserv->CountTicks(ticks, m_sifMan.get());
	m_usbd->CountTicks(ticks);
#endif
}

void CIopBios::NotifyVBlankStart()
{
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->status == THREAD_STATUS_WAIT_VBLANK_START)
		{
			thread->status = THREAD_STATUS_RUNNING;
			LinkThread(thread->id);
		}
	}
}

void CIopBios::NotifyVBlankEnd()
{
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->status == THREAD_STATUS_WAIT_VBLANK_END)
		{
			thread->status = THREAD_STATUS_RUNNING;
			LinkThread(thread->id);
		}
	}
#ifdef _IOP_EMULATE_MODULES
	m_cdvdfsv->ProcessCommands(m_sifMan.get());
	m_fileIo->ProcessCommands(m_sifMan.get());
#endif
}

uint32 CIopBios::CreateSemaphore(uint32 initialCount, uint32 maxCount, uint32 optionData, uint32 attributes)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: CreateSemaphore(initialCount = %i, maxCount = %i, optionData = 0x%08X, attributes = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), initialCount, maxCount, optionData, attributes);
#endif

	uint32 semaphoreId = m_semaphores.Allocate();
	assert(semaphoreId != -1);
	if(semaphoreId == -1)
	{
		return -1;
	}

	auto semaphore = m_semaphores[semaphoreId];
	semaphore->count = initialCount;
	semaphore->maxCount = maxCount;
	semaphore->id = semaphoreId;
	semaphore->waitCount = 0;
	semaphore->attrib = attributes;
	semaphore->option = optionData;

	return semaphore->id;
}

uint32 CIopBios::DeleteSemaphore(uint32 semaphoreId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: DeleteSemaphore(semaphoreId = %i);\r\n",
	                          m_currentThreadId.Get(), semaphoreId);
#endif

	auto semaphore = m_semaphores[semaphoreId];
	if(!semaphore)
	{
		CLog::GetInstance().Warn(LOGNAME, "%i: Warning, trying to access invalid semaphore with id %i.\r\n",
		                         m_currentThreadId.Get(), semaphoreId);
		return KERNEL_RESULT_ERROR_UNKNOWN_SEMAID;
	}

	if(semaphore->waitCount != 0)
	{
		while(semaphore->waitCount != 0)
		{
			bool changed = SemaReleaseSingleThread(semaphoreId, true);
			if(!changed) break;
		}
		m_rescheduleNeeded = true;
	}

	m_semaphores.Free(semaphoreId);

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::SignalSemaphore(uint32 semaphoreId, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: SignalSemaphore(semaphoreId = %d, inInterrupt = %d);\r\n",
	                          m_currentThreadId.Get(), semaphoreId, inInterrupt);
#endif

	auto semaphore = m_semaphores[semaphoreId];
	if(!semaphore)
	{
		CLog::GetInstance().Warn(LOGNAME, "%d: Warning, trying to access invalid semaphore with id %d.\r\n",
		                         m_currentThreadId.Get(), semaphoreId);
		return KERNEL_RESULT_ERROR_UNKNOWN_SEMAID;
	}

	if(semaphore->waitCount != 0)
	{
		SemaReleaseSingleThread(semaphoreId, false);
		if(!inInterrupt)
		{
			m_rescheduleNeeded = true;
		}
	}
	else
	{
		if(semaphore->count == semaphore->maxCount)
		{
			return KERNEL_RESULT_ERROR_SEMA_OVF;
		}
		else
		{
			semaphore->count++;
		}
	}
	return KERNEL_RESULT_OK;
}

uint32 CIopBios::WaitSemaphore(uint32 semaphoreId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: WaitSemaphore(semaphoreId = %d);\r\n",
	                          m_currentThreadId.Get(), semaphoreId);
#endif

	auto semaphore = m_semaphores[semaphoreId];
	if(!semaphore)
	{
		CLog::GetInstance().Warn(LOGNAME, "%d: Warning, trying to access invalid semaphore with id %d.\r\n",
		                         m_currentThreadId.Get(), semaphoreId);
		return KERNEL_RESULT_ERROR_UNKNOWN_SEMAID;
	}

	if(semaphore->count == 0)
	{
		uint32 threadId = m_currentThreadId;
		THREAD* thread = GetThread(threadId);
		thread->status = THREAD_STATUS_WAITING_SEMAPHORE;
		thread->waitSemaphore = semaphoreId;
		UnlinkThread(threadId);
		semaphore->waitCount++;
		m_rescheduleNeeded = true;
	}
	else
	{
		semaphore->count--;
	}
	return KERNEL_RESULT_OK;
}

uint32 CIopBios::PollSemaphore(uint32 semaphoreId)
{
	CLog::GetInstance().Print(LOGNAME, "%d: PollSemaphore(semaphoreId = %d);\r\n",
	                          m_currentThreadId.Get(), semaphoreId);

	auto semaphore = m_semaphores[semaphoreId];
	if(!semaphore)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_SEMAID;
	}

	if(semaphore->count == 0)
	{
		return KERNEL_RESULT_ERROR_SEMA_ZERO;
	}

	semaphore->count--;

	return 0;
}

uint32 CIopBios::ReferSemaphoreStatus(uint32 semaphoreId, uint32 statusPtr)
{
	CLog::GetInstance().Print(LOGNAME, "%d: ReferSemaphoreStatus(semaphoreId = %d, statusPtr = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), semaphoreId, statusPtr);

	auto semaphore = m_semaphores[semaphoreId];
	if(semaphore == nullptr)
	{
		return -1;
	}

	auto status = reinterpret_cast<SEMAPHORE_STATUS*>(m_ram + statusPtr);
	status->attrib = semaphore->attrib;
	status->option = semaphore->option;
	status->initCount = 0;
	status->maxCount = semaphore->maxCount;
	status->currentCount = semaphore->count;
	status->numWaitThreads = semaphore->waitCount;

	return 0;
}

bool CIopBios::SemaReleaseSingleThread(uint32 semaphoreId, bool deleted)
{
	auto semaphore = m_semaphores[semaphoreId];
	assert(semaphore);
	assert(semaphore->waitCount != 0);

	bool changed = false;
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->waitSemaphore == semaphoreId)
		{
			assert(thread->status == THREAD_STATUS_WAITING_SEMAPHORE);
			thread->context.gpr[CMIPS::V0] = deleted ? KERNEL_RESULT_ERROR_WAIT_DELETE : KERNEL_RESULT_OK;
			thread->status = THREAD_STATUS_RUNNING;
			LinkThread(thread->id);
			thread->waitSemaphore = 0;
			semaphore->waitCount--;
			changed = true;
			break;
		}
	}

	//Something went wrong if nothing changed
	assert(changed);
	return changed;
}

uint32 CIopBios::CreateEventFlag(uint32 attributes, uint32 options, uint32 initValue)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: CreateEventFlag(attr = 0x%08X, opt = 0x%08X, initValue = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), attributes, options, initValue);
#endif

	uint32 eventId = m_eventFlags.Allocate();
	assert(eventId != -1);
	if(eventId == -1)
	{
		return -1;
	}

	EVENTFLAG* eventFlag = m_eventFlags[eventId];

	eventFlag->id = eventId;
	eventFlag->value = initValue;
	eventFlag->options = options;
	eventFlag->attributes = attributes;

	return eventFlag->id;
}

uint32 CIopBios::DeleteEventFlag(uint32 eventId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: DeleteEventFlag(eventId = %d);\r\n",
	                          m_currentThreadId.Get(), eventId);
#endif

	auto eventFlag = m_eventFlags[eventId];
	if(!eventFlag)
	{
		CLog::GetInstance().Warn(LOGNAME, "%d: Warning, trying to access invalid event flag with id %d.\r\n",
		                         m_currentThreadId.Get(), eventId);
		return KERNEL_RESULT_ERROR_UNKNOWN_EVFID;
	}

	m_eventFlags.Free(eventId);

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::SetEventFlag(uint32 eventId, uint32 value, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: SetEventFlag(eventId = %d, value = 0x%08X, inInterrupt = %d);\r\n",
	                          m_currentThreadId.Get(), eventId, value, inInterrupt);
#endif

	auto eventFlag = m_eventFlags[eventId];
	if(eventFlag == nullptr)
	{
		return -1;
	}

	eventFlag->value |= value;

	//Check all threads waiting for this event
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->status != THREAD_STATUS_WAITING_EVENTFLAG) continue;
		if(thread->waitEventFlag == eventId)
		{
			bool success = ProcessEventFlag(thread->waitEventFlagMode, eventFlag->value, thread->waitEventFlagMask,
			                                (thread->waitEventFlagResultPtr != 0) ? reinterpret_cast<uint32*>(m_ram + thread->waitEventFlagResultPtr) : nullptr);
			if(success)
			{
				thread->waitEventFlag = 0;
				thread->waitEventFlagResultPtr = 0;

				thread->status = THREAD_STATUS_RUNNING;
				LinkThread(thread->id);

				if(!inInterrupt)
				{
					m_rescheduleNeeded = true;
				}
			}
		}
	}

	return 0;
}

uint32 CIopBios::ClearEventFlag(uint32 eventId, uint32 value)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ClearEventFlag(eventId = %d, value = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), eventId, value);
#endif

	EVENTFLAG* eventFlag = m_eventFlags[eventId];
	if(eventFlag == NULL)
	{
		return -1;
	}

	eventFlag->value &= value;

	return 0;
}

uint32 CIopBios::WaitEventFlag(uint32 eventId, uint32 value, uint32 mode, uint32 resultPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: WaitEventFlag(eventId = %d, value = 0x%08X, mode = 0x%02X, resultPtr = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), eventId, value, mode, resultPtr);
#endif

	auto eventFlag = m_eventFlags[eventId];
	if(eventFlag == nullptr)
	{
		return -1;
	}

	bool success = ProcessEventFlag(mode, eventFlag->value, value,
	                                (resultPtr != 0) ? reinterpret_cast<uint32*>(m_ram + resultPtr) : nullptr);
	if(!success)
	{
		auto thread = GetThread(m_currentThreadId);
		thread->status = THREAD_STATUS_WAITING_EVENTFLAG;
		UnlinkThread(thread->id);
		thread->waitEventFlag = eventId;
		thread->waitEventFlagMode = mode;
		thread->waitEventFlagMask = value;
		thread->waitEventFlagResultPtr = resultPtr;
		m_rescheduleNeeded = true;
	}

	return 0;
}

uint32 CIopBios::PollEventFlag(uint32 eventId, uint32 value, uint32 mode, uint32 resultPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: PollEventFlag(eventId = %d, value = 0x%08X, mode = 0x%02X, resultPtr = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), eventId, value, mode, resultPtr);
#endif

	auto eventFlag = m_eventFlags[eventId];
	if(!eventFlag)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_EVFID;
	}

	if(value == 0)
	{
		return KERNEL_RESULT_ERROR_EVF_ILLEGAL_PAT;
	}

	bool success = ProcessEventFlag(mode, eventFlag->value, value,
	                                (resultPtr != 0) ? reinterpret_cast<uint32*>(m_ram + resultPtr) : nullptr);
	if(!success)
	{
		return KERNEL_RESULT_ERROR_EVF_CONDITION;
	}

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::ReferEventFlagStatus(uint32 eventId, uint32 infoPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReferEventFlagStatus(eventId = %d, infoPtr = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), eventId, infoPtr);
#endif

	EVENTFLAG* eventFlag = m_eventFlags[eventId];
	if(eventFlag == NULL)
	{
		return -1;
	}

	if(infoPtr == 0)
	{
		return -1;
	}

	EVENTFLAGINFO* eventFlagInfo(reinterpret_cast<EVENTFLAGINFO*>(m_ram + infoPtr));
	eventFlagInfo->attributes = eventFlag->attributes;
	eventFlagInfo->options = eventFlag->options;
	eventFlagInfo->initBits = 0;
	eventFlagInfo->currBits = eventFlag->value;
	eventFlagInfo->numThreads = 0;

	return 0;
}

bool CIopBios::ProcessEventFlag(uint32 mode, uint32& value, uint32 mask, uint32* resultPtr)
{
	bool success = false;
	uint32 maskResult = value & mask;

	if(mode & WEF_OR)
	{
		success = (maskResult != 0);
	}
	else
	{
		success = (maskResult == mask);
	}

	if(success)
	{
		if(resultPtr)
		{
			*resultPtr = value;
		}

		if(mode & WEF_CLEAR)
		{
			value = 0;
		}
	}

	return success;
}

uint32 CIopBios::CreateMessageBox()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: CreateMessageBox();\r\n",
	                          m_currentThreadId.Get());
#endif

	uint32 boxId = m_messageBoxes.Allocate();
	assert(boxId != -1);
	if(boxId == -1)
	{
		return -1;
	}

	auto box = m_messageBoxes[boxId];
	box->nextMsgPtr = 0;
	box->numMessage = 0;

	return boxId;
}

uint32 CIopBios::DeleteMessageBox(uint32 boxId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: DeleteMessageBox(boxId = %d);\r\n",
	                          m_currentThreadId.Get(), boxId);
#endif

	auto box = m_messageBoxes[boxId];
	if(!box)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_MBXID;
	}

	m_messageBoxes.Free(boxId);

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::SendMessageBox(uint32 boxId, uint32 messagePtr, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: SendMessageBox(boxId = %d, messagePtr = 0x%08X, inInterrupt = %d);\r\n",
	                          m_currentThreadId.Get(), boxId, messagePtr, inInterrupt);
#endif

	auto box = m_messageBoxes[boxId];
	if(!box)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_MBXID;
	}

	//Check if there's a thread waiting for a message first
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->status != THREAD_STATUS_WAITING_MESSAGEBOX) continue;
		if(thread->waitMessageBox == boxId)
		{
			if(thread->waitMessageBoxResultPtr != 0)
			{
				uint32* result = reinterpret_cast<uint32*>(m_ram + thread->waitMessageBoxResultPtr);
				*result = messagePtr;
			}

			thread->waitMessageBox = 0;
			thread->waitMessageBoxResultPtr = 0;

			thread->status = THREAD_STATUS_RUNNING;
			LinkThread(thread->id);
			if(!inInterrupt)
			{
				m_rescheduleNeeded = true;
			}

			return KERNEL_RESULT_OK;
		}
	}

	//Check if message already exists in the linked list
	if(box->numMessage != 0)
	{
		auto msgPtr = &box->nextMsgPtr;
		while(1)
		{
			if((*msgPtr) == 0)
			{
				break;
			}
			msgPtr = reinterpret_cast<uint32*>(m_ram + *msgPtr);
			if((*msgPtr) == messagePtr)
			{
				//Message already queued in the message box.
				//Space Invaders: Invasion Day will trigger this and will end up corrupting the linked list.
				//We return an error here, but this is not confirmed if this is the proper behavior.
				CLog::GetInstance().Warn(LOGNAME, "Failed to send message: message already queued (boxId = %d, messagePtr = 0x%08X).\r\n",
				                         boxId, messagePtr);
				return -1;
			}
		}
	}

	auto header = reinterpret_cast<MESSAGE_HEADER*>(m_ram + messagePtr);
	header->nextMsgPtr = 0;

	auto msgPtr = &box->nextMsgPtr;
	while(1)
	{
		if((*msgPtr) == 0)
		{
			(*msgPtr) = messagePtr;
			break;
		}
		msgPtr = reinterpret_cast<uint32*>(m_ram + *msgPtr);
	}

	box->numMessage++;

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::ReceiveMessageBox(uint32 messagePtr, uint32 boxId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReceiveMessageBox(messagePtr = 0x%08X, boxId = %d);\r\n",
	                          m_currentThreadId.Get(), messagePtr, boxId);
#endif

	auto box = m_messageBoxes[boxId];
	if(!box)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_MBXID;
	}

	if(box->numMessage != 0)
	{
		uint32* message = reinterpret_cast<uint32*>(m_ram + messagePtr);
		(*message) = box->nextMsgPtr;

		//Unlink message
		auto header = reinterpret_cast<MESSAGE_HEADER*>(m_ram + box->nextMsgPtr);
		box->nextMsgPtr = header->nextMsgPtr;
		box->numMessage--;
	}
	else
	{
		THREAD* thread = GetThread(m_currentThreadId);
		thread->status = THREAD_STATUS_WAITING_MESSAGEBOX;
		UnlinkThread(thread->id);
		thread->waitMessageBox = boxId;
		thread->waitMessageBoxResultPtr = messagePtr;
		m_rescheduleNeeded = true;
	}

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::PollMessageBox(uint32 messagePtr, uint32 boxId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: PollMessageBox(messagePtr = 0x%08X, boxId = %d);\r\n",
	                          m_currentThreadId.Get(), messagePtr, boxId);
#endif

	auto box = m_messageBoxes[boxId];
	if(!box)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_MBXID;
	}

	if(box->numMessage == 0)
	{
		return KERNEL_RESULT_ERROR_MBX_NOMSG;
	}

	auto message = reinterpret_cast<uint32*>(m_ram + messagePtr);
	(*message) = box->nextMsgPtr;

	//Unlink message
	auto header = reinterpret_cast<MESSAGE_HEADER*>(m_ram + box->nextMsgPtr);
	box->nextMsgPtr = header->nextMsgPtr;
	box->numMessage--;

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::ReferMessageBoxStatus(uint32 boxId, uint32 statusPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReferMessageBox(boxId = %d, statusPtr = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), boxId, statusPtr);
#endif

	auto box = m_messageBoxes[boxId];
	if(!box)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_MBXID;
	}

	auto status = reinterpret_cast<MESSAGEBOX_STATUS*>(m_ram + statusPtr);
	status->attr = 0;
	status->option = 0;
	status->numMessage = box->numMessage;
	status->numWaitThread = 0;
	status->messagePtr = box->nextMsgPtr;

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::CreateFpl(uint32 paramPtr)
{
	auto param = reinterpret_cast<FPL_PARAM*>(m_ram + paramPtr);
	if((param->attr & ~FPL_ATTR_VALID_MASK) != 0)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_ATTR;
	}

	auto fplId = m_fpls.Allocate();
	assert(fplId != FplList::INVALID_ID);
	if(fplId == FplList::INVALID_ID)
	{
		return -1;
	}

	uint32 bitmapSize = (param->blockCount + 7) / 8;
	uint32 totalSize = (param->blockCount * param->blockSize) + bitmapSize;

	uint32 poolPtr = m_sysmem->AllocateMemory(totalSize, 0, 0);
	if(poolPtr == 0)
	{
		m_fpls.Free(fplId);
		return KERNEL_RESULT_ERROR_NO_MEMORY;
	}

	auto fpl = m_fpls[fplId];
	fpl->attr = param->attr;
	fpl->option = param->option;
	fpl->blockCount = param->blockCount;
	fpl->blockSize = param->blockSize;
	fpl->poolPtr = poolPtr;

	return fplId;
}

uint32 CIopBios::DeleteFpl(uint32 fplId)
{
	auto fpl = m_fpls[fplId];
	if(!fpl)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_FPLID;
	}

	m_sysmem->FreeMemory(fpl->poolPtr);
	m_fpls.Free(fplId);

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::AllocateFpl(uint32 fplId)
{
	uint32 result = pAllocateFpl(fplId);
	if(result == KERNEL_RESULT_ERROR_NO_MEMORY)
	{
		CLog::GetInstance().Warn(LOGNAME, "No memory left while calling AllocateFpl, should be waiting. (not implemented)");
		assert(false);
	}
	return result;
}

uint32 CIopBios::pAllocateFpl(uint32 fplId)
{
	auto fpl = m_fpls[fplId];
	if(!fpl)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_FPLID;
	}

	uint32 bitmapPtr = (fpl->blockCount * fpl->blockSize);
	auto bitmap = m_ram + fpl->poolPtr + bitmapPtr;

	for(uint32 blockIdx = 0; blockIdx < fpl->blockCount; blockIdx++)
	{
		uint32 bitmapByteIdx = blockIdx / 8;
		uint32 bitmapBitMask = 1 << (blockIdx % 8);
		uint8& bitmapByte = bitmap[bitmapByteIdx];
		if(bitmapByte & bitmapBitMask) continue;
		bitmapByte |= bitmapBitMask;
		return fpl->poolPtr + (blockIdx * fpl->blockSize);
	}

	return KERNEL_RESULT_ERROR_NO_MEMORY;
}

uint32 CIopBios::FreeFpl(uint32 fplId, uint32 blockPtr)
{
	auto fpl = m_fpls[fplId];
	if(!fpl)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_FPLID;
	}

	if(blockPtr < fpl->poolPtr)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_MEMBLOCK;
	}

	uint32 blockIdx = (blockPtr - fpl->poolPtr) / fpl->blockSize;
	if(blockIdx >= fpl->blockCount)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_MEMBLOCK;
	}

	uint32 bitmapPtr = (fpl->blockCount * fpl->blockSize);
	auto bitmap = m_ram + fpl->poolPtr + bitmapPtr;
	uint32 bitmapByteIdx = blockIdx / 8;
	uint32 bitmapBitMask = 1 << (blockIdx % 8);

	bitmap[bitmapByteIdx] &= ~bitmapBitMask;

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::CreateVpl(uint32 paramPtr)
{
	auto param = reinterpret_cast<VPL_PARAM*>(m_ram + paramPtr);
	if((param->attr & ~VPL_ATTR_VALID_MASK) != 0)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_ATTR;
	}

	auto vplId = m_vpls.Allocate();
	assert(vplId != VplList::INVALID_ID);
	if(vplId == VplList::INVALID_ID)
	{
		return -1;
	}

	auto headBlockId = m_memoryBlocks.Allocate();
	assert(headBlockId != MemoryBlockList::INVALID_ID);
	if(headBlockId == MemoryBlockList::INVALID_ID)
	{
		m_vpls.Free(vplId);
		return -1;
	}

	uint32 poolPtr = m_sysmem->AllocateMemory(param->size, 0, 0);
	if(poolPtr == 0)
	{
		//This seems to work on actual hardware (maybe fixed in later revisions)
		m_memoryBlocks.Free(headBlockId);
		m_vpls.Free(vplId);
		return KERNEL_RESULT_ERROR_NO_MEMORY;
	}

	auto vpl = m_vpls[vplId];
	vpl->attr = param->attr;
	vpl->option = param->option;
	vpl->poolPtr = poolPtr;
	vpl->size = param->size;
	vpl->headBlockId = headBlockId;

	auto headBlock = m_memoryBlocks[headBlockId];
	headBlock->nextBlockId = MemoryBlockList::INVALID_ID;
	headBlock->address = vpl->size;
	headBlock->size = 0;

	return vplId;
}

uint32 CIopBios::DeleteVpl(uint32 vplId)
{
	auto vpl = m_vpls[vplId];
	if(!vpl)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_VPLID;
	}

	m_sysmem->FreeMemory(vpl->poolPtr);

	//Free blocks
	auto nextBlockId = vpl->headBlockId;
	auto nextBlock = m_memoryBlocks[nextBlockId];
	while(nextBlock != nullptr)
	{
		uint32 currentBlockId = nextBlockId;
		nextBlockId = nextBlock->nextBlockId;
		nextBlock = m_memoryBlocks[nextBlockId];
		m_memoryBlocks.Free(currentBlockId);
	}

	m_vpls.Free(vplId);

	return 0;
}

uint32 CIopBios::pAllocateVpl(uint32 vplId, uint32 size)
{
	auto vpl = m_vpls[vplId];
	if(!vpl)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_VPLID;
	}

	//Aligned to 8 bytes
	int32 allocSize = (size + 7) & ~0x07;
	if(allocSize < 0)
	{
		return KERNEL_RESULT_ERROR_NO_MEMORY;
	}

	uint32 freeSize = GetVplFreeSize(vplId);
	if(allocSize > freeSize)
	{
		return KERNEL_RESULT_ERROR_NO_MEMORY;
	}

	uint32 begin = 0;
	auto nextBlockId = &vpl->headBlockId;
	auto nextBlock = m_memoryBlocks[*nextBlockId];
	while(nextBlock != nullptr)
	{
		uint32 end = nextBlock->address;
		if((end - begin) >= allocSize)
		{
			break;
		}
		begin = nextBlock->address + nextBlock->size;
		nextBlockId = &nextBlock->nextBlockId;
		nextBlock = m_memoryBlocks[*nextBlockId];
	}

	if(nextBlock != nullptr)
	{
		uint32 newBlockId = m_memoryBlocks.Allocate();
		assert(newBlockId != MemoryBlockList::INVALID_ID);
		if(newBlockId == MemoryBlockList::INVALID_ID)
		{
			return -1;
		}
		auto newBlock = m_memoryBlocks[newBlockId];
		newBlock->address = begin;
		newBlock->size = allocSize;
		newBlock->nextBlockId = *nextBlockId;
		*nextBlockId = newBlockId;
		return begin + vpl->poolPtr;
	}

	return KERNEL_RESULT_ERROR_ILLEGAL_MEMSIZE;
}

uint32 CIopBios::FreeVpl(uint32 vplId, uint32 ptr)
{
	auto vpl = m_vpls[vplId];
	if(!vpl)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_VPLID;
	}

	ptr -= vpl->poolPtr;
	//Search for block pointing at the address
	auto nextBlockId = &vpl->headBlockId;
	auto nextBlock = m_memoryBlocks[*nextBlockId];
	while(nextBlock != nullptr)
	{
		if(nextBlock->address == ptr)
		{
			break;
		}
		nextBlockId = &nextBlock->nextBlockId;
		nextBlock = m_memoryBlocks[*nextBlockId];
	}

	if(nextBlock != nullptr)
	{
		m_memoryBlocks.Free(*nextBlockId);
		*nextBlockId = nextBlock->nextBlockId;
	}
	else
	{
		return -1;
	}

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::ReferVplStatus(uint32 vplId, uint32 statPtr)
{
	auto vpl = m_vpls[vplId];
	if(!vpl)
	{
		return KERNEL_RESULT_ERROR_UNKNOWN_VPLID;
	}

	uint32 size = vpl->size - 40;
	uint32 freeSize = GetVplFreeSize(vplId);

	auto stat = reinterpret_cast<VPL_STATUS*>(m_ram + statPtr);
	stat->attr = vpl->attr;
	stat->option = vpl->option;
	stat->size = size;
	stat->freeSize = freeSize;

	return 0;
}

uint32 CIopBios::GetVplFreeSize(uint32 vplId)
{
	auto vpl = m_vpls[vplId];
	assert(vpl != nullptr);
	if(!vpl)
	{
		return 0;
	}

	uint32 size = vpl->size - 40;

	uint32 freeSize = size;
	auto nextBlockId = vpl->headBlockId;
	auto nextBlock = m_memoryBlocks[nextBlockId];
	while(nextBlock != nullptr)
	{
		if(nextBlock->nextBlockId == MemoryBlockList::INVALID_ID)
		{
			assert(nextBlock->address == vpl->size);
			break;
		}
		freeSize -= nextBlock->size;
		freeSize -= 8;
		nextBlockId = nextBlock->nextBlockId;
		nextBlock = m_memoryBlocks[nextBlockId];
	}

	return freeSize;
}

void CIopBios::WaitCdSync()
{
	uint32 threadId = m_currentThreadId;
	auto thread = GetThread(threadId);
	thread->status = THREAD_STATUS_WAIT_CDSYNC;
	UnlinkThread(threadId);
	m_rescheduleNeeded = true;
}

void CIopBios::ReleaseWaitCdSync()
{
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->status != THREAD_STATUS_WAIT_CDSYNC) continue;

		thread->status = THREAD_STATUS_RUNNING;
		LinkThread(thread->id);
	}
}

Iop::CSysmem* CIopBios::GetSysmem()
{
	return m_sysmem.get();
}

Iop::CIoman* CIopBios::GetIoman()
{
	return m_ioman.get();
}

Iop::CSifMan* CIopBios::GetSifman()
{
	return m_sifMan.get();
}

Iop::CSifCmd* CIopBios::GetSifcmd()
{
	return m_sifCmd.get();
}

Iop::CCdvdman* CIopBios::GetCdvdman()
{
	return m_cdvdman.get();
}

Iop::CLoadcore* CIopBios::GetLoadcore()
{
	return m_loadcore.get();
}

#ifdef _IOP_EMULATE_MODULES

Iop::CPadMan* CIopBios::GetPadman()
{
	return m_padman.get();
}

Iop::CCdvdfsv* CIopBios::GetCdvdfsv()
{
	return m_cdvdfsv.get();
}

Iop::CMcServ* CIopBios::GetMcServ()
{
	return static_cast<Iop::CMcServ*>(m_mcserv.get());
}

Iop::CUsbd* CIopBios::GetUsbd()
{
	return static_cast<Iop::CUsbd*>(m_usbd.get());
}

#endif

int32 CIopBios::RegisterIntrHandler(uint32 line, uint32 mode, uint32 handler, uint32 arg)
{
	assert(FindIntrHandler(line) == -1);
	if(FindIntrHandler(line) != -1)
	{
		return KERNEL_RESULT_ERROR_FOUND_HANDLER;
	}

	if(line >= Iop::CIntc::LINES_MAX)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_INTRCODE;
	}

	//Registering a null handler is a no-op
	if(handler == 0)
	{
		return KERNEL_RESULT_OK;
	}

	uint32 handlerId = m_intrHandlers.Allocate();
	assert(handlerId != -1);
	if(handlerId == -1)
	{
		return KERNEL_RESULT_ERROR;
	}

	auto intrHandler = m_intrHandlers[handlerId];
	intrHandler->line = line;
	intrHandler->mode = mode;
	intrHandler->handler = handler;
	intrHandler->arg = arg;

	return KERNEL_RESULT_OK;
}

int32 CIopBios::ReleaseIntrHandler(uint32 line)
{
	if(line >= Iop::CIntc::LINES_MAX)
	{
		return KERNEL_RESULT_ERROR_ILLEGAL_INTRCODE;
	}

	uint32 handlerId = FindIntrHandler(line);
	if(handlerId == -1)
	{
		return KERNEL_RESULT_ERROR_NOTFOUND_HANDLER;
	}

	m_intrHandlers.Free(handlerId);
	return KERNEL_RESULT_OK;
}

int32 CIopBios::FindIntrHandler(uint32 line)
{
	for(auto handlerIterator = std::begin(m_intrHandlers); handlerIterator != std::end(m_intrHandlers); handlerIterator++)
	{
		auto handler = m_intrHandlers[handlerIterator];
		if(!handler) continue;
		if(handler->line == line) return handlerIterator;
	}
	return -1;
}

uint32 CIopBios::AssembleThreadFinish(CMIPSAssembler& assembler)
{
	uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_EXITTHREAD);
	assembler.SYSCALL();
	return address;
}

uint32 CIopBios::AssembleReturnFromException(CMIPSAssembler& assembler)
{
	uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_RETURNFROMEXCEPTION);
	assembler.SYSCALL();
	return address;
}

uint32 CIopBios::AssembleIdleFunction(CMIPSAssembler& assembler)
{
	uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_RESCHEDULE);
	assembler.SYSCALL();
	return address;
}

uint32 CIopBios::AssembleModuleStarterProc(CMIPSAssembler& assembler)
{
	uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;

	//PROCESSMODULESTART sets some state in S0, S1, S2, S3 that is required by FINISHMODULESTART

	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_PROCESSMODULESTART);
	assembler.SYSCALL();

	//Save module start/stop result
	assembler.ADDIU(CMIPS::S4, CMIPS::V0, CMIPS::R0);

	//Wait a little bit before reporting result. If we initialize/load modules too fast,
	//there's a chance the current module won't have time to complete its init (through threads)
	//properly before another module starts. Capcom vs SNK2 has this problem where multiple
	//modules will call SdInit, but the order in which they are called is important.
	//Shin Megami Tensei: Nocturne is also sensitive to this delay.
	assembler.LI(CMIPS::A0, 0x4000);
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_DELAYTHREADTICKS);
	assembler.SYSCALL();

	//Restore module start/stop result
	assembler.ADDU(CMIPS::A0, CMIPS::S4, CMIPS::R0);

	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_FINISHMODULESTART);
	assembler.SYSCALL();

	return address;
}

uint32 CIopBios::AssembleAlarmThreadProc(CMIPSAssembler& assembler)
{
	uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
	auto delayThreadLabel = assembler.CreateLabel();

	//Prolog
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFF80);
	assembler.SW(CMIPS::RA, 0x10, CMIPS::SP);
	assembler.SW(CMIPS::S0, 0x14, CMIPS::SP);

	assembler.MOV(CMIPS::S0, CMIPS::A0); //S0 has the info struct ptr

	//Delay thread
	assembler.MarkLabel(delayThreadLabel);
	assembler.LW(CMIPS::A0, 0x08, CMIPS::S0);
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_DELAYTHREADTICKS);
	assembler.SYSCALL();

	//Call handler
	assembler.LW(CMIPS::V0, 0x00, CMIPS::S0);
	assembler.JALR(CMIPS::V0);
	assembler.LW(CMIPS::A0, 0x04, CMIPS::S0);

	assembler.BNE(CMIPS::V0, CMIPS::R0, delayThreadLabel);
	assembler.SW(CMIPS::V0, 0x08, CMIPS::S0);

	//Epilog
	assembler.LW(CMIPS::S0, 0x14, CMIPS::SP);
	assembler.LW(CMIPS::RA, 0x10, CMIPS::SP);

	assembler.JR(CMIPS::RA);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0x0080);

	return address;
}

uint32 CIopBios::AssembleVblankHandler(CMIPSAssembler& assembler)
{
	uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
	auto checkHandlerLabel = assembler.CreateLabel();
	auto moveToNextHandlerLabel = assembler.CreateLabel();

	int16 stackAlloc = 0x80;

	//Prolog
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
	assembler.SW(CMIPS::RA, 0x10, CMIPS::SP);
	assembler.SW(CMIPS::S0, 0x14, CMIPS::SP);

	assembler.MOV(CMIPS::S0, CMIPS::A0); //S0 = type
	assembler.MOV(CMIPS::S1, CMIPS::R0); //S1 = counter

	assembler.MarkLabel(checkHandlerLabel);
	assembler.LI(CMIPS::T0, BIOS_VBLANKHANDLER_BASE);
	assembler.SLL(CMIPS::T1, CMIPS::S1, 4); //Multiples of 0x10 bytes
	assembler.ADDU(CMIPS::T0, CMIPS::T0, CMIPS::T1);

	//Check isValid
	assembler.LW(CMIPS::T1, offsetof(VBLANKHANDLER, isValid), CMIPS::T0);
	assembler.BEQ(CMIPS::T1, CMIPS::R0, moveToNextHandlerLabel);
	assembler.NOP();

	//Check type
	assembler.LW(CMIPS::T1, offsetof(VBLANKHANDLER, type), CMIPS::T0);
	assembler.BNE(CMIPS::T1, CMIPS::S0, moveToNextHandlerLabel);
	assembler.NOP();

	assembler.LW(CMIPS::T1, offsetof(VBLANKHANDLER, handler), CMIPS::T0);
	assembler.JALR(CMIPS::T1);
	assembler.LW(CMIPS::A0, offsetof(VBLANKHANDLER, arg), CMIPS::T0);

	//TODO: Check return value

	assembler.MarkLabel(moveToNextHandlerLabel);
	assembler.ADDIU(CMIPS::S1, CMIPS::S1, 1);
	assembler.SLTIU(CMIPS::T0, CMIPS::S1, MAX_VBLANKHANDLER);

	assembler.BNE(CMIPS::T0, CMIPS::R0, checkHandlerLabel);
	assembler.NOP();

	//Epilog
	assembler.LW(CMIPS::S0, 0x14, CMIPS::SP);
	assembler.LW(CMIPS::RA, 0x10, CMIPS::SP);

	assembler.JR(CMIPS::RA);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);

	return address;
}

void CIopBios::HandleException()
{
	assert(m_cpu.m_State.nHasException == MIPS_EXCEPTION_SYSCALL);

	m_rescheduleNeeded = false;

	uint32 searchAddress = m_cpu.m_pAddrTranslator(&m_cpu, m_cpu.m_State.nCOP0[CCOP_SCU::EPC]);

	const auto* memoryMapElem = m_cpu.m_pMemoryMap->GetReadMap(searchAddress);
	assert(memoryMapElem != nullptr);
	assert(memoryMapElem->nType == CMemoryMap::MEMORYMAP_TYPE_MEMORY);
	auto memory = reinterpret_cast<const uint32*>(reinterpret_cast<uint8*>(memoryMapElem->pPointer) + (searchAddress - memoryMapElem->nStart));

	uint32 callInstruction = memory[0];
	if(callInstruction == 0x0000000C)
	{
		switch(m_cpu.m_State.nGPR[CMIPS::V0].nV0)
		{
		case SYSCALL_EXITTHREAD:
			ExitThread();
			break;
		case SYSCALL_RETURNFROMEXCEPTION:
			ReturnFromException();
			break;
		case SYSCALL_RESCHEDULE:
			Reschedule();
			break;
		case SYSCALL_SLEEPTHREAD:
			SleepThread();
			break;
		case SYSCALL_PROCESSMODULESTART:
			ProcessModuleStart();
			break;
		case SYSCALL_FINISHMODULESTART:
			FinishModuleStart();
			break;
		case SYSCALL_DELAYTHREADTICKS:
			DelayThreadTicks(m_cpu.m_State.nGPR[CMIPS::A0].nV0);
			break;
		}
	}
	else
	{
		//Search for the import record
		uint32 instruction = callInstruction;
		while(instruction != 0x41E00000)
		{
			memory--;
			instruction = memory[0];
		}
		uint32 functionId = callInstruction & 0xFFFF;
		FRAMEWORK_MAYBE_UNUSED uint32 version = memory[2];
		auto moduleName = ReadModuleName(reinterpret_cast<const uint8*>(memory + 3));

#ifdef _DEBUG
		if(moduleName == "libsd")
		{
			Iop::CLibSd::TraceCall(m_cpu, functionId);
		}
#endif

		auto module(m_modules.find(moduleName));
		if(module != m_modules.end())
		{
			module->second->Invoke(m_cpu, functionId);
		}
		else
		{
#ifdef _DEBUG
			CLog::GetInstance().Warn(LOGNAME, "%08X: Trying to call a function from non-existing module (%s, %d).\r\n",
			                         m_cpu.m_State.nPC, std::string(moduleName).c_str(), functionId);
#endif
		}
	}

	if(m_rescheduleNeeded)
	{
		assert((m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL) == 0);
		m_rescheduleNeeded = false;
		Reschedule();
	}

	m_cpu.m_State.nHasException = 0;
}

void CIopBios::HandleInterrupt()
{
	if(m_cpu.GenerateInterrupt(m_cpu.m_State.nPC))
	{
		//Find first concerned interrupt
		UNION64_32 status(
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::STATUS0),
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::STATUS1));
		UNION64_32 mask(
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::MASK0),
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::MASK1));
		status.f &= mask.f;
		assert(status.f != 0);
		if(status.f == 0)
		{
			ReturnFromException();
			return;
		}
		uint32 line = __builtin_ctzll(status.f);
		status.f = ~(1ULL << line);
		m_cpu.m_pMemoryMap->SetWord(Iop::CIntc::STATUS0, status.h0);
		m_cpu.m_pMemoryMap->SetWord(Iop::CIntc::STATUS1, status.h1);
		//Check if there's an handler to call
		{
			uint32 handlerId = FindIntrHandler(line);
			if(handlerId == -1)
			{
				ReturnFromException();
				return;
			}
			else
			{
				//Snap out of current thread
				if(m_currentThreadId != -1)
				{
					SaveThreadContext(m_currentThreadId);
				}
				m_currentThreadId = -1;
				auto handler = m_intrHandlers[handlerId];
				m_cpu.m_State.nPC = handler->handler;
				m_cpu.m_State.nGPR[CMIPS::SP].nD0 = (BIOS_INTSTACK_BASE + BIOS_INTSTACK_SIZE) - STACK_FRAME_RESERVE_SIZE;
				m_cpu.m_State.nGPR[CMIPS::A0].nD0 = static_cast<int32>(handler->arg);
				m_cpu.m_State.nGPR[CMIPS::RA].nD0 = static_cast<int32>(m_returnFromExceptionAddress);
			}
		}
	}
}

void CIopBios::ReturnFromException()
{
	uint32& status = m_cpu.m_State.nCOP0[CCOP_SCU::STATUS];
	assert(status & (CMIPS::STATUS_ERL | CMIPS::STATUS_EXL));
	if(status & CMIPS::STATUS_ERL)
	{
		status &= ~CMIPS::STATUS_ERL;
	}
	else if(status & CMIPS::STATUS_EXL)
	{
		status &= ~CMIPS::STATUS_EXL;
	}
	Reschedule();
}

void CIopBios::DeleteModules()
{
	m_modules.clear();

#ifdef _IOP_EMULATE_MODULES
	m_padman.reset();
	m_mtapman.reset();
	m_mcserv.reset();
	m_cdvdfsv.reset();
	m_fileIo.reset();
	m_hleModules.clear();
#endif
	m_sifCmd.reset();
	m_sifMan.reset();
	m_libsd.reset();
	m_stdio.reset();
	m_ioman.reset();
	m_sysmem.reset();
	m_modload.reset();
}

void CIopBios::UnloadUserComponents()
{
	//This will attempt to get rid of most things a game might have loaded
	//This also adds some constraints that could be annoying, like the inability
	//for HLE modules to create threads or semaphores
	//This won't get rid of some stuff such as memory allocations
	assert(m_currentThreadId == -1);
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		TerminateThread(thread->id);
		DeleteThread(thread->id);
	}
	for(auto loadedModuleIterator = std::begin(m_loadedModules);
	    loadedModuleIterator != std::end(m_loadedModules); loadedModuleIterator++)
	{
		auto loadedModule = *loadedModuleIterator;
		if(!loadedModule) continue;
		if(loadedModule->state == MODULE_STATE::STARTED)
		{
			loadedModule->state = MODULE_STATE::STOPPED;
		}
		UnloadModule(loadedModuleIterator);
	}
	std::experimental::erase_if(m_modules, [](const auto& modulePair) { return std::dynamic_pointer_cast<Iop::CDynamic>(modulePair.second); });
	m_intrHandlers.FreeAll();
	m_semaphores.FreeAll();
	m_sifCmd->ClearServers();
}

int32 CIopBios::LoadHleModule(const Iop::ModulePtr& module)
{
	auto loadedModuleId = SearchModuleByName(module->GetId().c_str());
	if(loadedModuleId >= 0)
	{
		return loadedModuleId;
	}

	loadedModuleId = m_loadedModules.Allocate();
	assert(loadedModuleId != -1);
	if(loadedModuleId == -1) return -1;

	auto loadedModule = m_loadedModules[loadedModuleId];
	strncpy(loadedModule->name, module->GetId().c_str(), LOADEDMODULE::MAX_NAME_SIZE);
	loadedModule->state = MODULE_STATE::HLE;

	//Register entries as if the module initialized itself
	RegisterHleModule(module);

	return loadedModuleId;
}

void CIopBios::RegisterHleModule(const Iop::ModulePtr& module)
{
	RegisterModule(module);
	if(auto sifModuleProvider = std::dynamic_pointer_cast<Iop::CSifModuleProvider>(module))
	{
		sifModuleProvider->RegisterSifModules(*m_sifMan);
	}
}

CIopBios::ModuleSet CIopBios::GetBuiltInModules() const
{
	//This gathers all built-in modules
	//We don't have a centralised place for them at the moment
	//and we only need this for save/load state
	ModuleSet modules;
	for(const auto& modulePair : m_modules)
	{
		//Exclude dynamic modules
		if(std::dynamic_pointer_cast<Iop::CDynamic>(modulePair.second))
		{
			continue;
		}
		modules.insert(modulePair.second.get());
	}
	for(const auto& modulePair : m_hleModules)
	{
		modules.insert(modulePair.second.get());
	}
	return modules;
}

std::string_view CIopBios::ReadModuleName(const uint8* memory)
{
	auto currChar = memory;
	uint32 size = 0;
	while(size < 8)
	{
		uint8 character = *(currChar++);
		if(character < 0x10) break;
		size++;
	}
	return std::string_view(reinterpret_cast<const char*>(memory), size);
}

std::string_view CIopBios::ReadModuleName(uint32 address)
{
	const auto* memoryMapElem = m_cpu.m_pMemoryMap->GetReadMap(address);
	assert(memoryMapElem != nullptr);
	assert(memoryMapElem->nType == CMemoryMap::MEMORYMAP_TYPE_MEMORY);
	auto memory = reinterpret_cast<const uint8*>(memoryMapElem->pPointer) + (address - memoryMapElem->nStart);
	return ReadModuleName(memory);
}

bool CIopBios::RegisterModule(const Iop::ModulePtr& module)
{
	bool registered = (m_modules.find(module->GetId()) != std::end(m_modules));
	if(registered) return false;
	m_modules[module->GetId()] = module;
	return true;
}

bool CIopBios::ReleaseModule(const std::string& moduleName)
{
	auto moduleIterator = m_modules.find(moduleName);
	if(moduleIterator == std::end(m_modules)) return false;
	m_modules.erase(moduleIterator);
	return true;
}

void CIopBios::RegisterHleModuleReplacement(const std::string& path, const Iop::ModulePtr& module)
{
	//Not definitive function, needs to support filtering by module name
	//Can also screw up some things with saved states
	m_hleModules[path] = module;
}

uint32 CIopBios::LoadExecutable(CELF32& elf, ExecutableRange& executableRange, uint32 baseAddress)
{
	unsigned int programHeaderIndex = GetElfProgramToLoad(elf);
	if(programHeaderIndex == -1)
	{
		throw std::runtime_error("No program to load.");
	}
	auto programHeader = elf.GetProgram(programHeaderIndex);
	if(baseAddress == ~0U)
	{
		baseAddress = m_sysmem->AllocateMemory(programHeader->nMemorySize, 0, 0);
	}
	memcpy(
	    m_ram + baseAddress,
	    elf.GetContent() + programHeader->nOffset,
	    programHeader->nFileSize);
	RelocateElf(elf, baseAddress, programHeader->nFileSize);

	executableRange.first = baseAddress;
	executableRange.second = baseAddress + programHeader->nMemorySize;

	return baseAddress + elf.GetHeader().nEntryPoint;
}

unsigned int CIopBios::GetElfProgramToLoad(CELF32& elf)
{
	unsigned int program = -1;
	const auto& header = elf.GetHeader();
	for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
	{
		auto programHeader = elf.GetProgram(i);
		if(programHeader != NULL && programHeader->nType == ELF::PT_LOAD)
		{
			if(program != -1)
			{
				throw std::runtime_error("Multiple loadable program headers found.");
			}
			program = i;
		}
	}
	return program;
}

void CIopBios::RelocateElf(CELF32& elf, uint32 programBaseAddress, uint32 programSize)
{
	//The IOP's ELF loader doesn't seem to follow the ELF standard completely
	//when it comes to relocations. The relocation function seems to use the
	//loaded program's base address for all adjustments. Using information from the
	//section headers (either the section's start address or info field) will yield
	//an incorrect result in some cases.
	//Examples:
	//- RWA.IRX module from Burnout 3 and Burnout Revenge
	//- Modules from Twinkle Star Sprites (stripped section name string table)
	const auto& header = elf.GetHeader();
	bool isVersion2 = (header.nType == ET_SCE_IOPRELEXEC2);
	auto programData = m_ram + programBaseAddress;
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
		const auto* sectionHeader = elf.GetSection(i);
		if(sectionHeader != nullptr && sectionHeader->nType == ELF::SHT_REL)
		{
			int32 lastHi16 = -1;
			uint32 instructionHi16 = -1;
			unsigned int recordCount = sectionHeader->nSize / 8;
			auto relocationRecord = reinterpret_cast<const uint32*>(elf.GetSectionData(i));
			uint32 sectionBase = 0;
			for(unsigned int record = 0; record < recordCount; record++)
			{
				//Helper to make sure we don't read/write things out of the module's memory area
				uint32 oobInstruction = 0;
				const auto& getInstructionRef = [&](int32 offset) -> uint32& {
					if((offset < 0) || (offset >= static_cast<int32>(programSize)))
					{
						CLog::GetInstance().Warn(LOGNAME, "Relocation %d accessing location out of bounds: %d.\r\n", record, offset);
						oobInstruction = 0;
						return oobInstruction;
					}
					return *reinterpret_cast<uint32*>(programData + offset);
				};

				//Some games have negative relocation addresses (Hitman 2: Silent Assassin)
				//Doesn't seem to be an issue, but we need offsets to be signed
				int32 relocationAddress = relocationRecord[0] - sectionBase;
				uint32 relocationType = relocationRecord[1] & 0xFF;
				{
					uint32& instruction = getInstructionRef(relocationAddress);
					switch(relocationType)
					{
					case ELF::R_MIPS_32:
					{
						instruction += programBaseAddress;
					}
					break;
					case ELF::R_MIPS_26:
					{
						uint32 offset = (instruction & 0x03FFFFFF) + (programBaseAddress >> 2);
						instruction &= ~0x03FFFFFF;
						instruction |= offset;
					}
					break;
					case ELF::R_MIPS_HI16:
						if(isVersion2)
						{
							assert((record + 1) != recordCount);
							assert((relocationRecord[3] & 0xFF) == ELF::R_MIPS_LO16);
							int32 nextRelocationAddress = relocationRecord[2] - sectionBase;
							uint32 nextInstruction = getInstructionRef(nextRelocationAddress);
							uint32 offset = static_cast<int16>(nextInstruction) + (instruction << 16);
							offset += programBaseAddress;
							if(offset & 0x8000) offset += 0x10000;
							instruction &= ~0xFFFF;
							instruction |= offset >> 16;
						}
						else
						{
							assert(lastHi16 == -1);
							lastHi16 = relocationAddress;
							instructionHi16 = instruction;
						}
						break;
					case ELF::R_MIPS_LO16:
						if(isVersion2)
						{
							uint32 offset = static_cast<int16>(instruction);
							offset += programBaseAddress;
							instruction &= ~0xFFFF;
							instruction |= offset & 0xFFFF;
						}
						else
						{
							assert(lastHi16 != -1);

							uint32 offset = static_cast<int16>(instruction) + (instructionHi16 << 16);
							offset += programBaseAddress;
							instruction &= ~0xFFFF;
							instruction |= offset & 0xFFFF;

							uint32& prevInstruction = getInstructionRef(lastHi16);
							prevInstruction &= ~0xFFFF;
							if(offset & 0x8000) offset += 0x10000;
							prevInstruction |= offset >> 16;
							lastHi16 = -1;
						}
						break;
					case R_MIPSSCE_MHI16:
					{
						assert(isVersion2);
						assert((record + 1) != recordCount);
						assert((relocationRecord[3] & 0xFF) == R_MIPSSCE_ADDEND);
						uint32 offset = relocationRecord[2] + programBaseAddress;
						if(offset & 0x8000) offset += 0x10000;
						offset >>= 16;
						while(1)
						{
							uint32& prevInstruction = getInstructionRef(relocationAddress);

							int32 mhiOffset = static_cast<int16>(prevInstruction);
							mhiOffset *= 4;

							prevInstruction &= ~0xFFFF;
							prevInstruction |= offset;

							if(mhiOffset == 0) break;
							relocationAddress += mhiOffset;
						}
					}
					break;
					default:
						//Some games use relocation types that might not be supported by the IOP's ELF loader
						//- R_MIPS_GPREL16: Used by Sega Ages 2500 Volume 8: Virtua Racing
						CLog::GetInstance().Warn(LOGNAME, "Unsupported ELF relocation type encountered (%d).\r\n",
						                         relocationType);
						assert(false);
						break;
					}
				}
				relocationRecord += 2;
			}
		}
	}
}

int32 CIopBios::TriggerCallback(uint32 address, uint32 arg0, uint32 arg1, uint32 arg2, uint32 arg3)
{
	// Call the addres on a callback thread with A0 set to arg0
	uint32 callbackThreadId = -1;

	//Find a thread we could recycle for a new callback
	for(auto thread : m_threads)
	{
		if(!thread) continue;
		if(thread->threadProc != address) continue;
		if(thread->status == THREAD_STATUS_DORMANT)
		{
			callbackThreadId = thread->id;
			break;
		}
	}

	//If no threads are available, create a new one
	if(callbackThreadId == -1)
	{
		callbackThreadId = CreateThread(address, DEFAULT_PRIORITY, DEFAULT_STACKSIZE, 0, 0);
	}

	StartThread(callbackThreadId, 0);
	ChangeThreadPriority(callbackThreadId, 1);

	auto thread = GetThread(callbackThreadId);
	thread->context.gpr[CMIPS::A0] = arg0;
	thread->context.gpr[CMIPS::A1] = arg1;
	thread->context.gpr[CMIPS::A2] = arg2;
	thread->context.gpr[CMIPS::A3] = arg3;

	return callbackThreadId;
}

void CIopBios::PopulateSystemIntcHandlers()
{
	auto intrHandlerTable = reinterpret_cast<CIopBios::SYSTEM_INTRHANDLER*>(&m_ram[BIOS_SYSTEM_INTRHANDLER_TABLE_BASE]);

	// homebrews expect this to be non-zero, to modify and use it as a callback during shadown
	// https://github.com/ps2dev/ps2sdk/blob/8b7579979db87ace4b0aa5693a8a560d15224a96/iop/dev9/poweroff/src/poweroff.c#L240
	auto cdvdIntrHandler = &intrHandlerTable[Iop::CIntc::LINES::LINE_CDROM];
	cdvdIntrHandler->handler = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Debug Stuff

#ifdef DEBUGGER_INCLUDED

#define TAGS_SECTION_IOP_MODULES ("modules")
#define TAGS_SECTION_IOP_MODULES_MODULE ("module")
#define TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS ("beginAddress")
#define TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS ("endAddress")
#define TAGS_SECTION_IOP_MODULES_MODULE_NAME ("name")

void CIopBios::LoadDebugTags(Framework::Xml::CNode* root)
{
	auto moduleSection = root->Select(TAGS_SECTION_IOP_MODULES);
	if(moduleSection == NULL) return;

	for(Framework::Xml::CFilteringNodeIterator nodeIterator(moduleSection, TAGS_SECTION_IOP_MODULES_MODULE);
	    !nodeIterator.IsEnd(); nodeIterator++)
	{
		auto moduleNode(*nodeIterator);
		const char* moduleName = moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_NAME);
		const char* beginAddress = moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS);
		const char* endAddress = moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS);
		if(!moduleName || !beginAddress || !endAddress) continue;
		if(FindModuleDebugInfo(moduleName) != std::end(m_moduleTags)) continue;

		BIOS_DEBUG_MODULE_INFO module;
		module.name = moduleName;
		module.begin = lexical_cast_hex<std::string>(beginAddress);
		module.end = lexical_cast_hex<std::string>(endAddress);
		module.param = NULL;
		m_moduleTags.push_back(module);
	}
}

void CIopBios::SaveDebugTags(Framework::Xml::CNode* root)
{
	auto moduleSection = std::make_unique<Framework::Xml::CNode>(TAGS_SECTION_IOP_MODULES, true);

	for(const auto& module : m_moduleTags)
	{
		auto moduleNode = std::make_unique<Framework::Xml::CNode>(TAGS_SECTION_IOP_MODULES_MODULE, true);
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS, lexical_cast_hex<std::string>(module.begin, 8).c_str());
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS, lexical_cast_hex<std::string>(module.end, 8).c_str());
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_NAME, module.name.c_str());
		moduleSection->InsertNode(std::move(moduleNode));
	}

	root->InsertNode(std::move(moduleSection));
}

BiosDebugModuleInfoArray CIopBios::GetModulesDebugInfo() const
{
	return m_moduleTags;
}

enum IOP_BIOS_DEBUG_OBJECT_TYPE
{
	IOP_BIOS_DEBUG_OBJECT_TYPE_SIFRPCSERVER = BIOS_DEBUG_OBJECT_TYPE_CUSTOM_START,
};

BiosDebugObjectInfoMap CIopBios::GetBiosObjectsDebugInfo() const
{
	static BiosDebugObjectInfoMap objectDebugInfo = [] {
		BiosDebugObjectInfoMap result;
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
			        {"State", BIOS_DEBUG_OBJECT_FIELD_TYPE::STRING},
			    };
			result.emplace(std::make_pair(BIOS_DEBUG_OBJECT_TYPE_THREAD, std::move(info)));
		}
		{
			BIOS_DEBUG_OBJECT_INFO info;
			info.name = "SIF RPC Servers";
			info.selectionAction = BIOS_DEBUG_OBJECT_ACTION::SHOW_LOCATION;
			info.fields =
			    {
			        {"Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::IDENTIFIER | BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::DATA_ADDRESS},
			        {"Handler", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::LOCATION | BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::TEXT_ADDRESS},
			        {"Queue Thread Id", BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::NONE},
			    };
			result.emplace(std::make_pair(IOP_BIOS_DEBUG_OBJECT_TYPE_SIFRPCSERVER, std::move(info)));
		}
		return result;
	}();
	return objectDebugInfo;
}

BiosDebugObjectArray CIopBios::GetBiosObjects(uint32 typeId) const
{
	BiosDebugObjectArray result;
	switch(typeId)
	{
	case BIOS_DEBUG_OBJECT_TYPE_THREAD:
		for(auto it = std::begin(m_threads); it != std::end(m_threads); it++)
		{
			auto thread = *it;
			if(!thread) continue;

			uint32 pc = 0;
			uint32 ra = 0;
			uint32 sp = 0;

			if(m_currentThreadId == it)
			{
				pc = m_cpu.m_State.nPC;
				ra = m_cpu.m_State.nGPR[CMIPS::RA].nV0;
				sp = m_cpu.m_State.nGPR[CMIPS::SP].nV0;
			}
			else
			{
				pc = thread->context.epc;
				ra = thread->context.gpr[CMIPS::RA];
				sp = thread->context.gpr[CMIPS::SP];
			}

			std::string stateDescription;
			int64 deltaTime = thread->nextActivateTime - GetCurrentTime();

			switch(thread->status)
			{
			case THREAD_STATUS_DORMANT:
				stateDescription = "Dormant";
				break;
			case THREAD_STATUS_RUNNING:
				if(deltaTime <= 0)
				{
					stateDescription = "Running";
				}
				else
				{
					stateDescription = string_format("Delayed (%ld ticks)", deltaTime);
				}
				break;
			case THREAD_STATUS_SLEEPING:
				stateDescription = "Sleeping";
				break;
			case THREAD_STATUS_WAITING_SEMAPHORE:
				stateDescription = string_format("Waiting (Semaphore: %d)", thread->waitSemaphore);
				break;
			case THREAD_STATUS_WAITING_EVENTFLAG:
				stateDescription = string_format("Waiting (Event Flag: %d)", thread->waitEventFlag);
				break;
			case THREAD_STATUS_WAITING_MESSAGEBOX:
				stateDescription = string_format("Waiting (Message Box: %d)", thread->waitMessageBox);
				break;
			case THREAD_STATUS_WAIT_VBLANK_START:
				stateDescription = "Waiting (Vblank Start)";
				break;
			case THREAD_STATUS_WAIT_VBLANK_END:
				stateDescription = "Waiting (Vblank End)";
				break;
			default:
				stateDescription = "Unknown";
				break;
			}

			BIOS_DEBUG_OBJECT obj;
			obj.fields = {
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, it),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, thread->priority),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, pc),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, ra),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, sp),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<std::string>, stateDescription),
			};
			result.push_back(obj);
		}
		break;
	case IOP_BIOS_DEBUG_OBJECT_TYPE_SIFRPCSERVER:
	{
		const auto& servers = m_sifCmd->GetServers();
		for(const auto& server : servers)
		{
			uint32 serverDataAddr = server->GetServerDataAddress();
			uint32 queueThreadId = -1;
			auto serverData = reinterpret_cast<const Iop::CSifCmd::SIFRPCSERVERDATA*>(m_ram + serverDataAddr);
			if(serverData->queueAddr != 0)
			{
				auto queueData = reinterpret_cast<const Iop::CSifCmd::SIFRPCQUEUEDATA*>(m_ram + serverData->queueAddr);
				queueThreadId = queueData->threadId;
			}
			BIOS_DEBUG_OBJECT obj;
			obj.fields = {
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, serverData->serverId),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, serverData->function),
			    BIOS_DEBUG_OBJECT_FIELD(std::in_place_type<uint32>, queueThreadId),
			};
			result.push_back(obj);
		}
	}
	break;
	}
	return result;
}

void CIopBios::PrepareModuleDebugInfo(CELF32& elf, const ExecutableRange& moduleRange, const std::string& moduleName, const std::string& modulePath)
{
	//Update module tag
	{
		auto moduleIterator(FindModuleDebugInfo(moduleRange.first, moduleRange.second));
		if(moduleIterator == std::end(m_moduleTags))
		{
			moduleIterator = FindModuleDebugInfo(moduleName);
			if(moduleIterator == std::end(m_moduleTags))
			{
				moduleIterator = m_moduleTags.insert(std::end(m_moduleTags), BIOS_DEBUG_MODULE_INFO());
			}
		}

		auto& module(*moduleIterator);
		module.name = moduleName;
		module.begin = moduleRange.first;
		module.end = moduleRange.second;
		module.param = NULL;
	}

	m_cpu.m_analysis->Analyse(moduleRange.first, moduleRange.second);

	bool functionAdded = false;
	bool variableAdded = false;

	//Look for import tables
	for(uint32 address = moduleRange.first; address < moduleRange.second; address += 4)
	{
		if(m_cpu.m_pMemoryMap->GetWord(address) == 0x41E00000)
		{
			if(m_cpu.m_pMemoryMap->GetWord(address + 4) != 0) continue;

			uint32 version = m_cpu.m_pMemoryMap->GetWord(address + 8);
			auto moduleName = ReadModuleName(address + 0xC);
			auto module(m_modules.find(moduleName));

			uint32 entryAddress = address + 0x14;
			while(m_cpu.m_pMemoryMap->GetWord(entryAddress) == 0x03E00008)
			{
				uint32 target = m_cpu.m_pMemoryMap->GetWord(entryAddress + 4);
				uint32 functionId = target & 0xFFFF;
				std::string functionName;
				if(moduleName == m_libsd->GetId())
				{
					functionName = m_libsd->GetFunctionName(functionId);
				}
				else if(module != m_modules.end())
				{
					functionName = (module->second)->GetFunctionName(functionId);
				}
				else
				{
					char functionNameTemp[256];
					sprintf(functionNameTemp, "unknown_%04X", functionId);
					functionName = functionNameTemp;
				}
				if(!m_cpu.m_Functions.Find(address))
				{
					auto fullFunctionName = std::string(moduleName) + "_" + functionName;
					m_cpu.m_Functions.InsertTag(entryAddress, std::move(fullFunctionName));
					functionAdded = true;
				}
				entryAddress += 8;
			}
		}
	}

	//Look also for symbol tables
	elf.EnumerateSymbols([&](const ELF::ELFSYMBOL32& symbol, uint8 type, uint8 binding, const char* name) {
		if(type == ELF::STT_FUNC)
		{
			m_cpu.m_Functions.InsertTag(symbol.nValue + moduleRange.first, name);
			functionAdded = true;
		}
		else if((type == ELF::STT_OBJECT) && (binding == ELF::STB_GLOBAL))
		{
			m_cpu.m_Variables.InsertTag(symbol.nValue + moduleRange.first, name);
			variableAdded = true;
		}
	});

	if(functionAdded)
	{
		m_cpu.m_Functions.OnTagListChange();
	}
	if(variableAdded)
	{
		m_cpu.m_Variables.OnTagListChange();
	}

	CLog::GetInstance().Print(LOGNAME, "Loaded IOP module '%s' @ 0x%08X.\r\n",
	                          modulePath.c_str(), moduleRange.first);
}

BiosDebugModuleInfoIterator CIopBios::FindModuleDebugInfo(const std::string& name)
{
	return std::find_if(std::begin(m_moduleTags), std::end(m_moduleTags),
	                    [=](const BIOS_DEBUG_MODULE_INFO& module) {
		                    return name == module.name;
	                    });
}

BiosDebugModuleInfoIterator CIopBios::FindModuleDebugInfo(uint32 beginAddress, uint32 endAddress)
{
	return std::find_if(std::begin(m_moduleTags), std::end(m_moduleTags),
	                    [=](const BIOS_DEBUG_MODULE_INFO& module) {
		                    return (beginAddress == module.begin) && (endAddress == module.end);
	                    });
}

#endif
