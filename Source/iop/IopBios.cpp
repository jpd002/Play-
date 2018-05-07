#include "IopBios.h"
#include "../COP_SCU.h"
#include "../Log.h"
#include "../ElfFile.h"
#include "../Ps2Const.h"
#include "../MipsExecutor.h"
#include "PtrStream.h"
#include "Iop_Intc.h"
#include "lexical_cast_ex.h"
#include <boost/lexical_cast.hpp>
#include <vector>
#include "xml/FilteringNodeIterator.h"
#include "../StructCollectionStateFile.h"

#ifdef _IOP_EMULATE_MODULES
#include "Iop_Cdvdfsv.h"
#include "Iop_McServ.h"
#include "Iop_FileIo.h"
#include "Iop_Naplink.h"
#endif

#include "Iop_SifManNull.h"
#include "Iop_SifModuleProvider.h"
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include "Iop_Thbase.h"
#include "Iop_Thsema.h"
#include "Iop_Thvpool.h"
#include "Iop_Thmsgbx.h"
#include "Iop_Thevent.h"
#include "Iop_Heaplib.h"
#include "Iop_Timrman.h"
#include "Iop_Intrman.h"
#include "Iop_Vblank.h"
#include "Iop_Dynamic.h"

#define LOGNAME "iop_bios"

#define STATE_MODULES ("iopbios/dyn_modules.xml")
#define STATE_MODULE_IMPORT_TABLE_ADDRESS ("ImportTableAddress")

#define BIOS_THREAD_LINK_HEAD_BASE (CIopBios::CONTROL_BLOCK_START + 0x0000)
#define BIOS_CURRENT_THREAD_ID_BASE (CIopBios::CONTROL_BLOCK_START + 0x0008)
#define BIOS_CURRENT_TIME_BASE (CIopBios::CONTROL_BLOCK_START + 0x0010)
#define BIOS_MODULESTARTREQUEST_HEAD_BASE (CIopBios::CONTROL_BLOCK_START + 0x0018)
#define BIOS_MODULESTARTREQUEST_FREE_BASE (CIopBios::CONTROL_BLOCK_START + 0x0020)
#define BIOS_HANDLERS_BASE (CIopBios::CONTROL_BLOCK_START + 0x0100)
#define BIOS_HANDLERS_END (BIOS_THREADS_BASE - 1)
#define BIOS_THREADS_BASE (CIopBios::CONTROL_BLOCK_START + 0x0200)
#define BIOS_THREADS_SIZE (sizeof(CIopBios::THREAD) * CIopBios::MAX_THREAD)
#define BIOS_SEMAPHORES_BASE (BIOS_THREADS_BASE + BIOS_THREADS_SIZE)
#define BIOS_SEMAPHORES_SIZE (sizeof(CIopBios::SEMAPHORE) * CIopBios::MAX_SEMAPHORE)
#define BIOS_EVENTFLAGS_BASE (BIOS_SEMAPHORES_BASE + BIOS_SEMAPHORES_SIZE)
#define BIOS_EVENTFLAGS_SIZE (sizeof(CIopBios::EVENTFLAG) * CIopBios::MAX_EVENTFLAG)
#define BIOS_INTRHANDLER_BASE (BIOS_EVENTFLAGS_BASE + BIOS_EVENTFLAGS_SIZE)
#define BIOS_INTRHANDLER_SIZE (sizeof(CIopBios::INTRHANDLER) * CIopBios::MAX_INTRHANDLER)
#define BIOS_MESSAGEBOX_BASE (BIOS_INTRHANDLER_BASE + BIOS_INTRHANDLER_SIZE)
#define BIOS_MESSAGEBOX_SIZE (sizeof(CIopBios::MESSAGEBOX) * CIopBios::MAX_MESSAGEBOX)
#define BIOS_VPL_BASE (BIOS_MESSAGEBOX_BASE + BIOS_MESSAGEBOX_SIZE)
#define BIOS_VPL_SIZE (sizeof(CIopBios::VPL) * CIopBios::MAX_VPL)
#define BIOS_MEMORYBLOCK_BASE (BIOS_VPL_BASE + BIOS_VPL_SIZE)
#define BIOS_MEMORYBLOCK_SIZE (sizeof(Iop::MEMORYBLOCK) * CIopBios::MAX_MEMORYBLOCK)
#define BIOS_MODULESTARTREQUEST_BASE (BIOS_MEMORYBLOCK_BASE + BIOS_MEMORYBLOCK_SIZE)
#define BIOS_MODULESTARTREQUEST_SIZE (sizeof(CIopBios::MODULESTARTREQUEST) * CIopBios::MAX_MODULESTARTREQUEST)
#define BIOS_LOADEDMODULE_BASE (BIOS_MODULESTARTREQUEST_BASE + BIOS_MODULESTARTREQUEST_SIZE)
#define BIOS_LOADEDMODULE_SIZE (sizeof(CIopBios::LOADEDMODULE) * CIopBios::MAX_LOADEDMODULE)
#define BIOS_CALCULATED_END (BIOS_LOADEDMODULE_BASE + BIOS_LOADEDMODULE_SIZE)

#define SYSCALL_EXITTHREAD 0x666
#define SYSCALL_RETURNFROMEXCEPTION 0x667
#define SYSCALL_RESCHEDULE 0x668
#define SYSCALL_SLEEPTHREAD 0x669
#define SYSCALL_PROCESSMODULESTART 0x66A
#define SYSCALL_FINISHMODULESTART 0x66B
#define SYSCALL_DELAYTHREADTICKS 0x66C

//This is the space needed to preserve at most four arguments in the stack frame (as per MIPS calling convention)
#define STACK_FRAME_RESERVE_SIZE 0x10

CIopBios::CIopBios(CMIPS& cpu, CMipsExecutor& cpuExecutor, uint8* ram, uint32 ramSize, uint8* spr)
    : m_cpu(cpu)
    , m_cpuExecutor(cpuExecutor)
    , m_ram(ram)
    , m_ramSize(ramSize)
    , m_spr(spr)
    , m_threadFinishAddress(0)
    , m_returnFromExceptionAddress(0)
    , m_idleFunctionAddress(0)
    , m_moduleStarterThreadProcAddress(0)
    , m_moduleStarterThreadId(0)
    , m_alarmThreadProcAddress(0)
    , m_threads(reinterpret_cast<THREAD*>(&m_ram[BIOS_THREADS_BASE]), 1, MAX_THREAD)
    , m_memoryBlocks(reinterpret_cast<Iop::MEMORYBLOCK*>(&ram[BIOS_MEMORYBLOCK_BASE]), 1, MAX_MEMORYBLOCK)
    , m_semaphores(reinterpret_cast<SEMAPHORE*>(&m_ram[BIOS_SEMAPHORES_BASE]), 1, MAX_SEMAPHORE)
    , m_eventFlags(reinterpret_cast<EVENTFLAG*>(&m_ram[BIOS_EVENTFLAGS_BASE]), 1, MAX_EVENTFLAG)
    , m_intrHandlers(reinterpret_cast<INTRHANDLER*>(&m_ram[BIOS_INTRHANDLER_BASE]), 1, MAX_INTRHANDLER)
    , m_messageBoxes(reinterpret_cast<MESSAGEBOX*>(&m_ram[BIOS_MESSAGEBOX_BASE]), 1, MAX_MESSAGEBOX)
    , m_vpls(reinterpret_cast<VPL*>(&m_ram[BIOS_VPL_BASE]), 1, MAX_VPL)
    , m_loadedModules(reinterpret_cast<LOADEDMODULE*>(&m_ram[BIOS_LOADEDMODULE_BASE]), 1, MAX_LOADEDMODULE)
    , m_currentThreadId(reinterpret_cast<uint32*>(m_ram + BIOS_CURRENT_THREAD_ID_BASE))
{
	static_assert(BIOS_CALCULATED_END <= CIopBios::CONTROL_BLOCK_END, "Control block size is too small");
}

CIopBios::~CIopBios()
{
	DeleteModules();
}

void CIopBios::Reset(const Iop::SifManPtr& sifMan)
{
	//Assemble handlers
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[BIOS_HANDLERS_BASE]));
		m_threadFinishAddress = AssembleThreadFinish(assembler);
		m_returnFromExceptionAddress = AssembleReturnFromException(assembler);
		m_idleFunctionAddress = AssembleIdleFunction(assembler);
		m_moduleStarterThreadProcAddress = AssembleModuleStarterThreadProc(assembler);
		m_alarmThreadProcAddress = AssembleAlarmThreadProc(assembler);
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
		m_ioman = std::make_shared<Iop::CIoman>(m_ram);
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
		m_fileIo = std::make_shared<Iop::CFileIo>(*m_sifMan, *m_ioman);
		RegisterModule(m_fileIo);
	}
	{
		m_cdvdfsv = std::make_shared<Iop::CCdvdfsv>(*m_sifMan, *m_cdvdman, m_ram);
		RegisterModule(m_cdvdfsv);
	}
	{
		m_mcserv = std::make_shared<Iop::CMcServ>(*m_sifMan);
		RegisterModule(m_mcserv);
	}
	//RegisterModule(std::make_shared<Iop::CNaplink>(*m_sifMan, *m_ioman));
	{
		m_padman = std::make_shared<Iop::CPadMan>();
		m_mtapman = std::make_shared<Iop::CMtapMan>();
	}
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

	m_sifMan->GenerateHandlers(m_ram, *m_sysmem);

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

void CIopBios::SaveState(Framework::CZipArchiveWriter& archive)
{
	CStructCollectionStateFile* modulesFile = new CStructCollectionStateFile(STATE_MODULES);
	{
		for(const auto& modulePair : m_modules)
		{
			if(auto dynamicModule = std::dynamic_pointer_cast<Iop::CDynamic>(modulePair.second))
			{
				CStructFile moduleStruct;
				{
					uint32 importTableAddress = reinterpret_cast<uint8*>(dynamicModule->GetExportTable()) - m_ram;
					moduleStruct.SetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS, importTableAddress);
				}
				modulesFile->InsertStruct(dynamicModule->GetId().c_str(), moduleStruct);
			}
		}
	}
	archive.InsertFile(modulesFile);

	m_sifCmd->SaveState(archive);
	m_cdvdman->SaveState(archive);
	m_loadcore->SaveState(archive);
#ifdef _IOP_EMULATE_MODULES
	m_fileIo->SaveState(archive);
	m_padman->SaveState(archive);
	m_cdvdfsv->SaveState(archive);
#endif
}

void CIopBios::LoadState(Framework::CZipArchiveReader& archive)
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

	CStructCollectionStateFile modulesFile(*archive.BeginReadFile(STATE_MODULES));
	{
		for(auto structIterator(modulesFile.GetStructBegin());
		    structIterator != modulesFile.GetStructEnd(); structIterator++)
		{
			const CStructFile& structFile(structIterator->second);
			uint32 importTableAddress = structFile.GetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS);
			auto module = std::make_shared<Iop::CDynamic>(reinterpret_cast<uint32*>(m_ram + importTableAddress));
			bool result = RegisterModule(module);
			assert(result);
		}
	}

	m_sifCmd->LoadState(archive);
	m_cdvdman->LoadState(archive);
	m_loadcore->LoadState(archive);
#ifdef _IOP_EMULATE_MODULES
	m_fileIo->LoadState(archive);
	m_padman->LoadState(archive);
	m_cdvdfsv->LoadState(archive);
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
	ModuleStartRequestHead() = 0;
	ModuleStartRequestFree() = BIOS_MODULESTARTREQUEST_BASE;

	//Initialize Module Load Request Free List
	for(unsigned int i = 0; i < (MAX_MODULESTARTREQUEST - 1); i++)
	{
		auto moduleStartRequest = reinterpret_cast<MODULESTARTREQUEST*>(m_ram + BIOS_MODULESTARTREQUEST_BASE) + i;
		moduleStartRequest->nextPtr = reinterpret_cast<uint8*>(moduleStartRequest + 1) - m_ram;
	}

	m_moduleStarterThreadId = CreateThread(m_moduleStarterThreadProcAddress, MODULE_INIT_PRIORITY, DEFAULT_STACKSIZE, 0, 0);
	StartThread(m_moduleStarterThreadId, 0);
}

void CIopBios::RequestModuleStart(bool stopRequest, uint32 moduleId, const char* path, const char* args, unsigned int argsLength)
{
	uint32 requestPtr = ModuleStartRequestFree();
	assert(requestPtr != 0);
	if(requestPtr == 0)
	{
		CLog::GetInstance().Print(LOGNAME, "Too many modules to be loaded.");
		return;
	}

	auto moduleStartRequest = reinterpret_cast<MODULESTARTREQUEST*>(m_ram + requestPtr);

	//Unlink from free list and link in active list (at the end)
	{
		ModuleStartRequestFree() = moduleStartRequest->nextPtr;

		uint32* currentPtr = &ModuleStartRequestHead();
		while(*currentPtr != 0)
		{
			auto currentModuleLoadRequest = reinterpret_cast<MODULESTARTREQUEST*>(m_ram + *currentPtr);
			currentPtr = &currentModuleLoadRequest->nextPtr;
		}

		*currentPtr = requestPtr;

		moduleStartRequest->nextPtr = 0;
	}

	moduleStartRequest->moduleId = moduleId;
	moduleStartRequest->stopRequest = stopRequest;

	assert((strlen(path) + 1) <= MODULESTARTREQUEST::MAX_PATH_SIZE);
	strncpy(moduleStartRequest->path, path, MODULESTARTREQUEST::MAX_PATH_SIZE);
	moduleStartRequest->path[MODULESTARTREQUEST::MAX_PATH_SIZE - 1] = 0;

	memcpy(moduleStartRequest->args, args, argsLength);
	moduleStartRequest->argsLength = argsLength;

	//Make sure thread runs at proper priority (Burnout 3 changes priority)
	ChangeThreadPriority(m_moduleStarterThreadId, MODULE_INIT_PRIORITY);
	WakeupThread(m_moduleStarterThreadId, false);
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

	assert(m_currentThreadId == m_moduleStarterThreadId);

	uint32 requestPtr = ModuleStartRequestHead();
	assert(requestPtr != 0);
	if(requestPtr == 0)
	{
		CLog::GetInstance().Print(LOGNAME, "Asked to load module when none was requested.");
		return;
	}

	auto moduleStartRequest = reinterpret_cast<MODULESTARTREQUEST*>(m_ram + requestPtr);

	//Unlink from active list and link in free list
	{
		ModuleStartRequestHead() = moduleStartRequest->nextPtr;

		moduleStartRequest->nextPtr = ModuleStartRequestFree();
		ModuleStartRequestFree() = requestPtr;
	}

	assert(m_currentThreadId == m_moduleStarterThreadId);

	//Reset stack pointer
	{
		auto thread = GetThread(m_moduleStarterThreadId);
		assert(thread);
		m_cpu.m_State.nGPR[CMIPS::SP].nV0 = thread->stackBase + thread->stackSize - STACK_FRAME_RESERVE_SIZE;
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

	m_cpu.m_State.nGPR[CMIPS::SP].nV0 -= 4;

	m_cpu.m_State.nGPR[CMIPS::S0].nV0 = moduleStartRequest->moduleId;
	m_cpu.m_State.nGPR[CMIPS::S1].nV0 = moduleStartRequest->stopRequest;
	m_cpu.m_State.nGPR[CMIPS::GP].nV0 = loadedModule->gp;
	m_cpu.m_State.nGPR[CMIPS::RA].nV0 = m_cpu.m_State.nPC;
	m_cpu.m_State.nPC = loadedModule->entryPoint;
}

void CIopBios::FinishModuleStart()
{
	uint32 moduleId = m_cpu.m_State.nGPR[CMIPS::S0].nV0;
	uint32 stopRequest = m_cpu.m_State.nGPR[CMIPS::S1].nV0;
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

	//We need to notify the EE that the load request is over
	m_sifMan->SendCallReply(Iop::CLoadcore::MODULE_ID, nullptr);
}

int32 CIopBios::LoadModule(const char* path)
{
#ifdef _IOP_EMULATE_MODULES
	//HACK: This is needed to make 'doom.elf' read input properly
	if(
	    !strcmp(path, "rom0:SIO2MAN") ||
	    !strcmp(path, "rom0:PADMAN") ||
	    !strcmp(path, "rom0:XSIO2MAN") ||
	    !strcmp(path, "rom0:XPADMAN"))
	{
		return LoadHleModule(m_padman);
	}
	if(!strcmp(path, "rom0:XMTAPMAN"))
	{
		return LoadHleModule(m_mtapman);
	}
	if(
	    !strcmp(path, "rom0:XMCMAN") ||
	    !strcmp(path, "rom0:XMCSERV"))
	{
		return LoadHleModule(m_mcserv);
	}
#endif
	uint32 handle = m_ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, path);
	if(handle & 0x80000000)
	{
		CLog::GetInstance().Print(LOGNAME, "Tried to load '%s' which couldn't be found.\r\n", path);
		return -1;
	}
	Iop::CIoman::CFile file(handle, *m_ioman);
	auto stream = m_ioman->GetFileStream(file);
	CElfFile module(*stream);
	return LoadModule(module, path);
}

int32 CIopBios::LoadModule(uint32 modulePtr)
{
	CELF module(m_ram + modulePtr);
	return LoadModule(module, "");
}

int32 CIopBios::LoadModuleFromHost(uint8* modulePtr)
{
	CELF module(modulePtr);
	return LoadModule(module, "");
}

int32 CIopBios::LoadModule(CELF& elf, const char* path)
{
	uint32 loadedModuleId = m_loadedModules.Allocate();
	assert(loadedModuleId != -1);
	if(loadedModuleId == -1) return -1;

	auto loadedModule = m_loadedModules[loadedModuleId];

	ExecutableRange moduleRange;
	uint32 entryPoint = LoadExecutable(elf, moduleRange);

	//Find .iopmod section
	const ELFHEADER& header(elf.GetHeader());
	const IOPMOD* iopMod = NULL;
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
		ELFSECTIONHEADER* sectionHeader(elf.GetSection(i));
		if(sectionHeader->nType != IOPMOD_SECTION_ID) continue;
		iopMod = reinterpret_cast<const IOPMOD*>(elf.GetSectionData(i));
	}

	std::string moduleName = iopMod ? iopMod->moduleName : "";
	if(moduleName.empty())
	{
		moduleName = path;
	}

	//Fill in module info
	strncpy(loadedModule->name, moduleName.c_str(), LOADEDMODULE::MAX_NAME_SIZE);
	loadedModule->start = moduleRange.first;
	loadedModule->end = moduleRange.second;
	loadedModule->entryPoint = entryPoint;
	loadedModule->gp = iopMod ? (iopMod->gp + moduleRange.first) : 0;
	loadedModule->state = MODULE_STATE::STOPPED;

#ifdef DEBUGGER_INCLUDED
	PrepareModuleDebugInfo(elf, moduleRange, moduleName, path);
#endif

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

	//Patch for Final Fantasy X PSF set ------------------------------
	if(strstr(path, "ffxpatch.irx") != NULL)
	{
		const uint32 patchAddress = moduleRange.first + 0x113C8;
		uint32 instruction = m_cpu.m_pMemoryMap->GetWord(patchAddress);
		if(instruction == 0x03E00008)
		{
			m_cpu.m_pMemoryMap->SetWord(patchAddress, 0x00000000);
		}
	}

	return loadedModuleId;
}

int32 CIopBios::UnloadModule(uint32 loadedModuleId)
{
	auto loadedModule = m_loadedModules[loadedModuleId];
	if(loadedModule == nullptr)
	{
		CLog::GetInstance().Print(LOGNAME, "UnloadModule failed because specified module (%d) doesn't exist.\r\n",
		                          loadedModuleId);
		return -1;
	}
	if(loadedModule->state != MODULE_STATE::STOPPED)
	{
		CLog::GetInstance().Print(LOGNAME, "UnloadModule failed because specified module (%d) wasn't stopped.\r\n",
		                          loadedModuleId);
		return -1;
	}

	//TODO: Remove module from IOP module list?
	//TODO: Invalidate MIPS analysis range?
	m_cpuExecutor.ClearActiveBlocksInRange(loadedModule->start, loadedModule->end);

	//TODO: Check return value here.
	m_sysmem->FreeMemory(loadedModule->start);
	m_loadedModules.Free(loadedModuleId);
	return loadedModuleId;
}

int32 CIopBios::StartModule(uint32 loadedModuleId, const char* path, const char* args, uint32 argsLength)
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
	RequestModuleStart(false, loadedModuleId, path, args, argsLength);
	return loadedModuleId;
}

int32 CIopBios::StopModule(uint32 loadedModuleId)
{
	auto loadedModule = m_loadedModules[loadedModuleId];
	if(loadedModule == nullptr)
	{
		CLog::GetInstance().Print(LOGNAME, "StopModule failed because specified module (%d) doesn't exist.\r\n",
		                          loadedModuleId);
		return -1;
	}
	if(loadedModule->state != MODULE_STATE::STARTED)
	{
		CLog::GetInstance().Print(LOGNAME, "StopModule failed because specified module (%d) wasn't started.\r\n",
		                          loadedModuleId);
		return -1;
	}
	if(loadedModule->residentState != MODULE_RESIDENT_STATE::REMOVABLE_RESIDENT_END)
	{
		CLog::GetInstance().Print(LOGNAME, "StopModule failed because specified module (%d) isn't removable.\r\n",
		                          loadedModuleId);
		return -1;
	}
	RequestModuleStart(true, loadedModuleId, "other", nullptr, 0);
	return loadedModuleId;
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
	return -1;
}

void CIopBios::ProcessModuleReset(const std::string& imagePath)
{
	unsigned int imageVersion = 1000;
	bool found = TryGetImageVersionFromPath(imagePath, &imageVersion);
	if(!found) found = TryGetImageVersionFromContents(imagePath, &imageVersion);
	assert(found);
	m_loadcore->SetModuleVersion(imageVersion);
#ifdef _IOP_EMULATE_MODULES
	m_fileIo->SetModuleVersion(imageVersion);
#endif
}

bool CIopBios::TryGetImageVersionFromPath(const std::string& imagePath, unsigned int* result)
{
	struct IMAGE_FILE_PATTERN
	{
		const char* start;
		const char* pattern;
	};
	static const IMAGE_FILE_PATTERN g_imageFilePatterns[] =
	    {
	        {"IOPRP", "IOPRP%d.IMG;1"},
	        {"DNAS", "DNAS%d.IMG;1"}};

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
	//Format of imagePath can be something like 'rom0:/UDNL cdrom0:/something;1'
	auto imagePathStart = strstr(imagePath.c_str(), "cdrom0:");
	if(!imagePathStart) return false;

	int32 fd = m_ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, imagePathStart);
	if(fd < 0) return false;

	Iop::CIoman::CFile file(fd, *m_ioman);
	auto stream = m_ioman->GetFileStream(file);
	while(1)
	{
		static const unsigned int moduleVersionStringSize = 0x10;
		char moduleVersionString[moduleVersionStringSize + 1];
		auto currentPos = stream->Tell();
		stream->Read(moduleVersionString, moduleVersionStringSize);
		moduleVersionString[moduleVersionStringSize] = 0;
		if(!strncmp(moduleVersionString, "PsIIfileio  ", 12))
		{
			//Found something
			unsigned int imageVersion = atoi(moduleVersionString + 12);
			if(imageVersion < 1000) return false;
			if(result)
			{
				(*result) = imageVersion;
			}
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
	CLog::GetInstance().Print(LOGNAME, "%d: CreateThread(threadProc = 0x%08X, priority = %d, stackSize = 0x%08X);\r\n",
	                          m_currentThreadId.Get(), threadProc, priority, stackSize);
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
	memset(&thread->context, 0, sizeof(thread->context));
	thread->context.delayJump = MIPS_INVALID_PC;
	thread->stackSize = stackSize;
	thread->stackBase = stackBase;
	memset(m_ram + thread->stackBase, 0, thread->stackSize);
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
		CLog::GetInstance().Print(LOGNAME, "%d: Failed to start thread %d, thread not dormant.\r\n",
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
		CLog::GetInstance().Print(LOGNAME, "%d: Failed to start thread %d, thread not dormant.\r\n",
		                          m_currentThreadId.Get(), threadId);
		assert(false);
		return -1;
	}

	static const auto pushToStack =
	    [](uint8* dst, uint32& stackAddress, const uint8* src, uint32 size) {
		    uint32 fixedSize = ((size + 0x3) & ~0x3);
		    uint32 copyAddress = stackAddress - size;
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

	//Returns negative value on failure
	return 0;
}

uint32 CIopBios::CancelAlarm(uint32 alarmFunction, uint32 param)
{
	//TODO: This needs to garantee that the alarm handler function won't be called after the cancel

	uint32 alarmThreadId = -1;

	for(auto thread : m_threads)
	{
		if(!thread) continue;
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
	threadInfo->attributes = 0;
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

	THREAD* thread = GetThread(threadId);
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

	assert(thread->status == THREAD_STATUS_SLEEPING);

	thread->status = THREAD_STATUS_RUNNING;
	LinkThread(threadId);
	if(!inInterrupt)
	{
		m_rescheduleNeeded = true;
	}

	return KERNEL_RESULT_OK;
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

uint64 CIopBios::GetCurrentTime()
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
	m_cdvdman->ProcessCommands();
	m_fileIo->ProcessCommands();
#endif
}

uint32 CIopBios::CreateSemaphore(uint32 initialCount, uint32 maxCount)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: CreateSemaphore(initialCount = %i, maxCount = %i);\r\n",
	                          m_currentThreadId.Get(), initialCount, maxCount);
#endif

	uint32 semaphoreId = m_semaphores.Allocate();
	assert(semaphoreId != -1);
	if(semaphoreId == -1)
	{
		return -1;
	}

	SEMAPHORE* semaphore = m_semaphores[semaphoreId];

	semaphore->count = initialCount;
	semaphore->maxCount = maxCount;
	semaphore->id = semaphoreId;
	semaphore->waitCount = 0;

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
		CLog::GetInstance().Print(LOGNAME, "%i: Warning, trying to access invalid semaphore with id %i.\r\n",
		                          m_currentThreadId.Get(), semaphoreId);
		return KERNEL_RESULT_ERROR_UNKNOWN_SEMAID;
	}

	assert(semaphore->waitCount == 0);
	m_semaphores.Free(semaphoreId);

	return KERNEL_RESULT_OK;
}

uint32 CIopBios::SignalSemaphore(uint32 semaphoreId, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: SignalSemaphore(semaphoreId = %d, inInterrupt = %d);\r\n",
	                          m_currentThreadId.Get(), semaphoreId, inInterrupt);
#endif

	SEMAPHORE* semaphore = m_semaphores[semaphoreId];
	if(semaphore == NULL)
	{
		CLog::GetInstance().Print(LOGNAME, "%d: Warning, trying to access invalid semaphore with id %d.\r\n",
		                          m_currentThreadId.Get(), semaphoreId);
		return -1;
	}

	if(semaphore->waitCount != 0)
	{
		for(auto thread : m_threads)
		{
			if(!thread) continue;
			if(thread->waitSemaphore == semaphoreId)
			{
				if(thread->status != THREAD_STATUS_WAITING_SEMAPHORE)
				{
					throw std::runtime_error("Thread not waiting for semaphone (inconsistent state).");
				}
				thread->status = THREAD_STATUS_RUNNING;
				LinkThread(thread->id);
				thread->waitSemaphore = 0;
				if(!inInterrupt)
				{
					m_rescheduleNeeded = true;
				}
				semaphore->waitCount--;
				if(semaphore->waitCount == 0)
				{
					break;
				}
			}
		}
	}
	else
	{
		semaphore->count++;
	}
	return 0;
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
		CLog::GetInstance().Print(LOGNAME, "%d: Warning, trying to access invalid semaphore with id %d.\r\n",
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
	status->attrib = 0;
	status->option = 0;
	status->initCount = 0;
	status->maxCount = semaphore->maxCount;
	status->currentCount = semaphore->count;
	status->numWaitThreads = semaphore->waitCount;

	return 0;
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
		CLog::GetInstance().Print(LOGNAME, "%d: Warning, trying to access invalid event flag with id %d.\r\n",
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

	if(box->nextMsgPtr != 0)
	{
		assert(box->numMessage > 0);

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

	if(box->nextMsgPtr == 0)
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

Iop::CIoman* CIopBios::GetIoman()
{
	return m_ioman.get();
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

uint32 CIopBios::FindIntrHandler(uint32 line)
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
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, STACK_FRAME_RESERVE_SIZE);
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

uint32 CIopBios::AssembleModuleStarterThreadProc(CMIPSAssembler& assembler)
{
	uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;

	auto startLabel = assembler.CreateLabel();

	assembler.MarkLabel(startLabel);
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_SLEEPTHREAD);
	assembler.SYSCALL();
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_PROCESSMODULESTART);
	assembler.SYSCALL();
	assembler.ADDU(CMIPS::A0, CMIPS::V0, CMIPS::R0);
	assembler.ADDIU(CMIPS::V0, CMIPS::R0, SYSCALL_FINISHMODULESTART);
	assembler.SYSCALL();
	assembler.BEQ(CMIPS::R0, CMIPS::R0, startLabel);
	assembler.NOP();

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

void CIopBios::HandleException()
{
	assert(m_cpu.m_State.nHasException == MIPS_EXCEPTION_SYSCALL);

	m_rescheduleNeeded = false;

	uint32 searchAddress = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
	uint32 callInstruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
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
			searchAddress -= 4;
			instruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
		}
		uint32 functionId = callInstruction & 0xFFFF;
		uint32 version = m_cpu.m_pMemoryMap->GetWord(searchAddress + 8);
		std::string moduleName = ReadModuleName(searchAddress + 0x0C);

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
			CLog::GetInstance().Print(LOGNAME, "%08X: Trying to call a function from non-existing module (%s, %d).\r\n",
			                          m_cpu.m_State.nPC, moduleName.c_str(), functionId);
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
		unsigned int line = -1;
		UNION64_32 status(
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::STATUS0),
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::STATUS1));
		UNION64_32 mask(
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::MASK0),
		    m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::MASK1));
		status.f &= mask.f;
		for(unsigned int i = 0; i < 0x40; i++)
		{
			if(status.f & (1LL << i))
			{
				line = i;
				break;
			}
		}
		assert(line != -1);
		if(line == -1)
		{
			ReturnFromException();
			return;
		}
		status.f = ~(1LL << line);
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
				INTRHANDLER* handler = m_intrHandlers[handlerId];
				m_cpu.m_State.nPC = handler->handler;
				m_cpu.m_State.nGPR[CMIPS::SP].nD0 -= STACK_FRAME_RESERVE_SIZE;
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

	m_sifCmd.reset();
	m_sifMan.reset();
	m_libsd.reset();
	m_stdio.reset();
	m_ioman.reset();
	m_sysmem.reset();
	m_modload.reset();
#ifdef _IOP_EMULATE_MODULES
	m_padman.reset();
	m_mtapman.reset();
	m_mcserv.reset();
	m_cdvdfsv.reset();
	m_fileIo.reset();
#endif
}

int32 CIopBios::LoadHleModule(const Iop::ModulePtr& module)
{
	auto loadedModuleId = SearchModuleByName(module->GetId().c_str());
	if(loadedModuleId != -1)
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
	RegisterModule(module);
	if(auto sifModuleProvider = std::dynamic_pointer_cast<Iop::CSifModuleProvider>(module))
	{
		sifModuleProvider->RegisterSifModules(*m_sifMan);
	}

	return loadedModuleId;
}

std::string CIopBios::ReadModuleName(uint32 address)
{
	std::string moduleName;
	const CMemoryMap::MEMORYMAPELEMENT* memoryMapElem = m_cpu.m_pMemoryMap->GetReadMap(address);
	assert(memoryMapElem != NULL);
	assert(memoryMapElem->nType == CMemoryMap::MEMORYMAP_TYPE_MEMORY);
	uint8* memory = reinterpret_cast<uint8*>(memoryMapElem->pPointer) + (address - memoryMapElem->nStart);
	while(1)
	{
		uint8 character = *(memory++);
		if(character == 0) break;
		if(character < 0x10) continue;
		moduleName += character;
	}
	return moduleName;
}

bool CIopBios::RegisterModule(const Iop::ModulePtr& module)
{
	bool registered = (m_modules.find(module->GetId()) != std::end(m_modules));
	if(registered) return false;
	m_modules[module->GetId()] = module;
	return true;
}

uint32 CIopBios::LoadExecutable(CELF& elf, ExecutableRange& executableRange)
{
	unsigned int programHeaderIndex = GetElfProgramToLoad(elf);
	if(programHeaderIndex == -1)
	{
		throw std::runtime_error("No program to load.");
	}
	ELFPROGRAMHEADER* programHeader = elf.GetProgram(programHeaderIndex);
	uint32 baseAddress = m_sysmem->AllocateMemory(programHeader->nMemorySize, 0, 0);
	RelocateElf(elf, baseAddress);

	memcpy(
	    m_ram + baseAddress,
	    elf.GetContent() + programHeader->nOffset,
	    programHeader->nFileSize);

	executableRange.first = baseAddress;
	executableRange.second = baseAddress + programHeader->nMemorySize;

	//Clean BSS sections
	{
		const ELFHEADER& header(elf.GetHeader());
		for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
		{
			ELFSECTIONHEADER* sectionHeader = elf.GetSection(i);
			if(sectionHeader->nType == CELF::SHT_NOBITS && sectionHeader->nStart != 0)
			{
				memset(m_ram + baseAddress + sectionHeader->nStart, 0, sectionHeader->nSize);
			}
		}
	}

	return baseAddress + elf.GetHeader().nEntryPoint;
}

unsigned int CIopBios::GetElfProgramToLoad(CELF& elf)
{
	unsigned int program = -1;
	const ELFHEADER& header = elf.GetHeader();
	for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
	{
		ELFPROGRAMHEADER* programHeader = elf.GetProgram(i);
		if(programHeader != NULL && programHeader->nType == 1)
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

void CIopBios::RelocateElf(CELF& elf, uint32 baseAddress)
{
	//The IOP's ELF loader doesn't seem to follow the ELF standard completely
	//when it comes to relocations. The relocation function seems to use the
	//.text section's base address for all adjustments. Using information from the
	//section headers (either the section's start address or info field) will yield
	//an incorrect result in some cases (ex.: RWA.IRA module from Burnout 3 and Burnout Revenge)

	const auto& header = elf.GetHeader();
	uint32 maxRelocAddress =
	    [&]() {
		    auto programHeader = elf.GetProgram(1);
		    if(!programHeader) return UINT32_MAX;
		    if(programHeader->nType != CELF::PT_LOAD) return UINT32_MAX;
		    return programHeader->nMemorySize;
	    }();
	bool isVersion2 = (header.nType == ET_SCE_IOPRELEXEC2);
	auto textSectionIndex = elf.FindSectionIndex(".text");
	assert(textSectionIndex != 0);
	auto textSection = elf.GetSection(textSectionIndex);
	auto textSectionData = reinterpret_cast<uint8*>(const_cast<void*>(elf.GetSectionData(textSectionIndex)));
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
		const auto* sectionHeader = elf.GetSection(i);
		if(sectionHeader != nullptr && sectionHeader->nType == CELF::SHT_REL)
		{
			uint32 lastHi16 = -1;
			uint32 instructionHi16 = -1;
			unsigned int recordCount = sectionHeader->nSize / 8;
			auto relocationRecord = reinterpret_cast<const uint32*>(elf.GetSectionData(i));
			uint32 sectionBase = 0;
			for(unsigned int record = 0; record < recordCount; record++)
			{
				uint32 relocationAddress = relocationRecord[0] - sectionBase;
				uint32 relocationType = relocationRecord[1] & 0xFF;
				assert(relocationAddress < maxRelocAddress);
				if(relocationAddress < maxRelocAddress)
				{
					uint32& instruction = *reinterpret_cast<uint32*>(&textSectionData[relocationAddress]);
					switch(relocationType)
					{
					case CELF::R_MIPS_32:
					{
						instruction += baseAddress;
					}
					break;
					case CELF::R_MIPS_26:
					{
						uint32 offset = (instruction & 0x03FFFFFF) + (baseAddress >> 2);
						instruction &= ~0x03FFFFFF;
						instruction |= offset;
					}
					break;
					case CELF::R_MIPS_HI16:
						if(isVersion2)
						{
							assert((record + 1) != recordCount);
							assert((relocationRecord[3] & 0xFF) == CELF::R_MIPS_LO16);
							uint32 nextRelocationAddress = relocationRecord[2] - sectionBase;
							uint32 nextInstruction = *reinterpret_cast<uint32*>(&textSectionData[nextRelocationAddress]);
							uint32 offset = static_cast<int16>(nextInstruction) + (instruction << 16);
							offset += baseAddress;
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
					case CELF::R_MIPS_LO16:
						if(isVersion2)
						{
							uint32 offset = static_cast<int16>(instruction);
							offset += baseAddress;
							instruction &= ~0xFFFF;
							instruction |= offset & 0xFFFF;
						}
						else
						{
							assert(lastHi16 != -1);

							uint32 offset = static_cast<int16>(instruction) + (instructionHi16 << 16);
							offset += baseAddress;
							instruction &= ~0xFFFF;
							instruction |= offset & 0xFFFF;

							uint32& prevInstruction = *reinterpret_cast<uint32*>(&textSectionData[lastHi16]);
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
						uint32 offset = relocationRecord[2] + baseAddress;
						if(offset & 0x8000) offset += 0x10000;
						offset >>= 16;
						while(1)
						{
							uint32& prevInstruction = *reinterpret_cast<uint32*>(&textSectionData[relocationAddress]);

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
						CLog::GetInstance().Print(LOGNAME, "Unsupported ELF relocation type encountered (%d).\r\n",
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

void CIopBios::TriggerCallback(uint32 address, uint32 arg0, uint32 arg1)
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
	auto moduleSection = new Framework::Xml::CNode(TAGS_SECTION_IOP_MODULES, true);

	for(const auto& module : m_moduleTags)
	{
		auto moduleNode = new Framework::Xml::CNode(TAGS_SECTION_IOP_MODULES_MODULE, true);
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS, lexical_cast_hex<std::string>(module.begin, 8).c_str());
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS, lexical_cast_hex<std::string>(module.end, 8).c_str());
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_NAME, module.name.c_str());
		moduleSection->InsertNode(moduleNode);
	}

	root->InsertNode(moduleSection);
}

BiosDebugModuleInfoArray CIopBios::GetModulesDebugInfo() const
{
	return m_moduleTags;
}

BiosDebugThreadInfoArray CIopBios::GetThreadsDebugInfo() const
{
	BiosDebugThreadInfoArray threadInfos;

	for(auto thread : m_threads)
	{
		if(!thread) continue;

		BIOS_DEBUG_THREAD_INFO threadInfo;
		threadInfo.id = thread->id;
		threadInfo.priority = thread->priority;
		if(m_currentThreadId == threadInfo.id)
		{
			threadInfo.pc = m_cpu.m_State.nPC;
			threadInfo.ra = m_cpu.m_State.nGPR[CMIPS::RA].nV0;
			threadInfo.sp = m_cpu.m_State.nGPR[CMIPS::SP].nV0;
		}
		else
		{
			threadInfo.pc = thread->context.epc;
			threadInfo.ra = thread->context.gpr[CMIPS::RA];
			threadInfo.sp = thread->context.gpr[CMIPS::SP];
		}

		switch(thread->status)
		{
		case THREAD_STATUS_DORMANT:
			threadInfo.stateDescription = "Dormant";
			break;
		case THREAD_STATUS_RUNNING:
			threadInfo.stateDescription = "Running";
			break;
		case THREAD_STATUS_SLEEPING:
			threadInfo.stateDescription = "Sleeping";
			break;
		case THREAD_STATUS_WAITING_SEMAPHORE:
			threadInfo.stateDescription = "Waiting (Semaphore: " + boost::lexical_cast<std::string>(thread->waitSemaphore) + ")";
			break;
		case THREAD_STATUS_WAITING_EVENTFLAG:
			threadInfo.stateDescription = "Waiting (Event Flag: " + boost::lexical_cast<std::string>(thread->waitEventFlag) + ")";
			break;
		case THREAD_STATUS_WAITING_MESSAGEBOX:
			threadInfo.stateDescription = "Waiting (Message Box: " + boost::lexical_cast<std::string>(thread->waitMessageBox) + ")";
			break;
		case THREAD_STATUS_WAIT_VBLANK_START:
			threadInfo.stateDescription = "Waiting (Vblank Start)";
			break;
		case THREAD_STATUS_WAIT_VBLANK_END:
			threadInfo.stateDescription = "Waiting (Vblank End)";
			break;
		default:
			threadInfo.stateDescription = "Unknown";
			break;
		}

		threadInfos.push_back(threadInfo);
	}

	return threadInfos;
}

void CIopBios::PrepareModuleDebugInfo(CELF& elf, const ExecutableRange& moduleRange, const std::string& moduleName, const std::string& modulePath)
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
	//Look for import tables
	for(uint32 address = moduleRange.first; address < moduleRange.second; address += 4)
	{
		if(m_cpu.m_pMemoryMap->GetWord(address) == 0x41E00000)
		{
			if(m_cpu.m_pMemoryMap->GetWord(address + 4) != 0) continue;

			uint32 version = m_cpu.m_pMemoryMap->GetWord(address + 8);
			std::string moduleName = ReadModuleName(address + 0xC);
			IopModuleMapType::iterator module(m_modules.find(moduleName));

			size_t moduleNameLength = moduleName.length();
			uint32 entryAddress = address + 0x0C + ((moduleNameLength + 3) & ~0x03);
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
				if(m_cpu.m_Functions.Find(address) == NULL)
				{
					m_cpu.m_Functions.InsertTag(entryAddress, (std::string(moduleName) + "_" + functionName).c_str());
					functionAdded = true;
				}
				entryAddress += 8;
			}
		}
	}

	//Look also for symbol tables
	{
		ELFSECTIONHEADER* pSymTab = elf.FindSection(".symtab");
		if(pSymTab != NULL)
		{
			const char* pStrTab = reinterpret_cast<const char*>(elf.GetSectionData(pSymTab->nIndex));
			if(pStrTab != NULL)
			{
				const ELFSYMBOL* pSym = reinterpret_cast<const ELFSYMBOL*>(elf.FindSectionData(".symtab"));
				unsigned int nCount = pSymTab->nSize / sizeof(ELFSYMBOL);

				for(unsigned int i = 0; i < nCount; i++)
				{
					if((pSym[i].nInfo & 0x0F) != 0x02) continue;
					ELFSECTIONHEADER* symbolSection = elf.GetSection(pSym[i].nSectionIndex);
					if(symbolSection == NULL) continue;
					m_cpu.m_Functions.InsertTag(moduleRange.first + symbolSection->nStart + pSym[i].nValue, (char*)pStrTab + pSym[i].nName);
					functionAdded = true;
				}
			}
		}
	}

	if(functionAdded)
	{
		m_cpu.m_Functions.OnTagListChange();
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
