#include "IopBios.h"
#include "../COP_SCU.h"
#include "../Log.h"
#include "../ElfFile.h"
#include "../Ps2Const.h"
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
#endif

#include "Iop_SifManNull.h"
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include "Iop_Thbase.h"
#include "Iop_Thsema.h"
#include "Iop_Thmsgbx.h"
#include "Iop_Thevent.h"
#include "Iop_Timrman.h"
#include "Iop_Intrman.h"
#include "Iop_Vblank.h"

#define LOGNAME "iop_bios"

#define STATE_MODULES						("iopbios/dyn_modules.xml")
#define STATE_MODULE_IMPORT_TABLE_ADDRESS	("ImportTableAddress")

#define BIOS_THREAD_LINK_HEAD_BASE			(CIopBios::CONTROL_BLOCK_START + 0x0000)
#define BIOS_CURRENT_THREAD_ID_BASE			(CIopBios::CONTROL_BLOCK_START + 0x0008)
#define BIOS_CURRENT_TIME_BASE				(CIopBios::CONTROL_BLOCK_START + 0x0010)
#define BIOS_MODULESTARTREQUEST_HEAD_BASE	(CIopBios::CONTROL_BLOCK_START + 0x0018)
#define BIOS_MODULESTARTREQUEST_FREE_BASE	(CIopBios::CONTROL_BLOCK_START + 0x0020)
#define BIOS_HANDLERS_BASE					(CIopBios::CONTROL_BLOCK_START + 0x0100)
#define BIOS_HANDLERS_END					(BIOS_THREADS_BASE - 1)
#define BIOS_THREADS_BASE					(CIopBios::CONTROL_BLOCK_START + 0x0200)
#define BIOS_THREADS_SIZE					(sizeof(CIopBios::THREAD) * CIopBios::MAX_THREAD)
#define BIOS_SEMAPHORES_BASE				(BIOS_THREADS_BASE + BIOS_THREADS_SIZE)
#define BIOS_SEMAPHORES_SIZE				(sizeof(CIopBios::SEMAPHORE) * CIopBios::MAX_SEMAPHORE)
#define BIOS_EVENTFLAGS_BASE				(BIOS_SEMAPHORES_BASE + BIOS_SEMAPHORES_SIZE)
#define BIOS_EVENTFLAGS_SIZE				(sizeof(CIopBios::EVENTFLAG) * CIopBios::MAX_EVENTFLAG)
#define BIOS_INTRHANDLER_BASE				(BIOS_EVENTFLAGS_BASE + BIOS_EVENTFLAGS_SIZE)
#define BIOS_INTRHANDLER_SIZE				(sizeof(CIopBios::INTRHANDLER) * CIopBios::MAX_INTRHANDLER)
#define BIOS_MESSAGEBOX_BASE				(BIOS_INTRHANDLER_BASE + BIOS_INTRHANDLER_SIZE)
#define BIOS_MESSAGEBOX_SIZE				(sizeof(CIopBios::MESSAGEBOX) * CIopBios::MAX_MESSAGEBOX)
#define BIOS_HEAPBLOCK_BASE					(BIOS_MESSAGEBOX_BASE + BIOS_MESSAGEBOX_SIZE)
#define BIOS_HEAPBLOCK_SIZE					(sizeof(Iop::CSysmem::BLOCK) * Iop::CSysmem::MAX_BLOCKS)
#define BIOS_MODULESTARTREQUEST_BASE		(BIOS_HEAPBLOCK_BASE + BIOS_HEAPBLOCK_SIZE)
#define BIOS_MODULESTARTREQUEST_SIZE		(sizeof(CIopBios::MODULESTARTREQUEST) * CIopBios::MAX_MODULESTARTREQUEST)
#define BIOS_LOADEDMODULE_BASE				(BIOS_MODULESTARTREQUEST_BASE + BIOS_MODULESTARTREQUEST_SIZE)
#define BIOS_LOADEDMODULE_SIZE				(sizeof(CIopBios::LOADEDMODULE) * CIopBios::MAX_LOADEDMODULE)
#define BIOS_CALCULATED_END					(BIOS_LOADEDMODULE_BASE + BIOS_LOADEDMODULE_SIZE)

#define SYSCALL_EXITTHREAD				0x666
#define SYSCALL_RETURNFROMEXCEPTION		0x667
#define SYSCALL_RESCHEDULE				0x668
#define SYSCALL_SLEEPTHREAD				0x669
#define SYSCALL_PROCESSMODULESTART		0x66A
#define SYSCALL_FINISHMODULESTART		0x66B
#define SYSCALL_DELAYTHREADTICKS		0x66C

//This is the space needed to preserve at most four arguments in the stack frame (as per MIPS calling convention)
#define STACK_FRAME_RESERVE_SIZE		0x10

#define ERROR_SEMAPHORE_ZERO	(-419)

CIopBios::CIopBios(CMIPS& cpu, uint8* ram, uint32 ramSize) 
: m_cpu(cpu)
, m_ram(ram)
, m_ramSize(ramSize)
, m_sifMan(nullptr)
, m_sifCmd(nullptr)
, m_stdio(nullptr)
, m_sysmem(nullptr)
, m_ioman(nullptr)
, m_modload(nullptr)
, m_cdvdman(nullptr)
, m_loadcore(nullptr)
#ifdef _IOP_EMULATE_MODULES
, m_padman(nullptr)
, m_cdvdfsv(nullptr)
#endif
, m_threadFinishAddress(0)
, m_returnFromExceptionAddress(0)
, m_idleFunctionAddress(0)
, m_moduleStarterThreadProcAddress(0)
, m_moduleStarterThreadId(0)
, m_alarmThreadProcAddress(0)
, m_threads(reinterpret_cast<THREAD*>(&m_ram[BIOS_THREADS_BASE]), 1, MAX_THREAD)
, m_semaphores(reinterpret_cast<SEMAPHORE*>(&m_ram[BIOS_SEMAPHORES_BASE]), 1, MAX_SEMAPHORE)
, m_eventFlags(reinterpret_cast<EVENTFLAG*>(&m_ram[BIOS_EVENTFLAGS_BASE]), 1, MAX_EVENTFLAG)
, m_intrHandlers(reinterpret_cast<INTRHANDLER*>(&m_ram[BIOS_INTRHANDLER_BASE]), 1, MAX_INTRHANDLER)
, m_messageBoxes(reinterpret_cast<MESSAGEBOX*>(&m_ram[BIOS_MESSAGEBOX_BASE]), 1, MAX_MESSAGEBOX)
, m_loadedModules(reinterpret_cast<LOADEDMODULE*>(&m_ram[BIOS_LOADEDMODULE_BASE]), 1, MAX_LOADEDMODULE)
{
	static_assert(BIOS_CALCULATED_END <= CIopBios::CONTROL_BLOCK_END, "Control block size is too small");
}

CIopBios::~CIopBios()
{
	DeleteModules();
}

void CIopBios::Reset(Iop::CSifMan* sifMan)
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
	CurrentThreadId() = -1;

	m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_IE;

	m_threads.FreeAll();
	m_semaphores.FreeAll();
	m_intrHandlers.FreeAll();
#ifdef DEBUGGER_INCLUDED
	m_moduleTags.clear();
#endif

	DeleteModules();

	if(sifMan == NULL)
	{
		m_sifMan = new Iop::CSifManNull();
	}
	else
	{
		m_sifMan = sifMan;
	}

	//Register built-in modules
	{
		m_ioman = new Iop::CIoman(m_ram);
		RegisterModule(m_ioman);
	}
	{
		m_stdio = new Iop::CStdio(m_ram, *m_ioman);
		RegisterModule(m_stdio);
	}
	{
		m_sysmem = new Iop::CSysmem(m_ram, CONTROL_BLOCK_END, m_ramSize, BIOS_HEAPBLOCK_BASE, *m_stdio, *m_ioman, *m_sifMan);
		RegisterModule(m_sysmem);
	}
	{
		m_modload = new Iop::CModload(*this, m_ram);
		RegisterModule(m_modload);
	}
	{
		RegisterModule(new Iop::CSysclib(m_ram, *m_stdio));
	}
	{
		m_loadcore = new Iop::CLoadcore(*this, m_ram, *m_sifMan);
		RegisterModule(m_loadcore);
	}
	{
		m_libsd = std::make_shared<Iop::CLibSd>();
	}
	{
		RegisterModule(new Iop::CThbase(*this, m_ram));
	}
	{
		RegisterModule(new Iop::CThmsgbx(*this, m_ram));
	}
	{
		RegisterModule(new Iop::CThsema(*this, m_ram));
	}
	{
		RegisterModule(new Iop::CThevent(*this, m_ram));
	}
	{
		RegisterModule(new Iop::CTimrman(*this));
	}
	{
		RegisterModule(new Iop::CIntrman(*this, m_ram));
	}
	{
		RegisterModule(new Iop::CVblank(*this));
	}
	{
		m_cdvdman = new Iop::CCdvdman(*this, m_ram);
		RegisterModule(m_cdvdman);
	}
	{
		RegisterModule(m_sifMan);
	}
	{
		m_sifCmd = new Iop::CSifCmd(*this, *m_sifMan, *m_sysmem, m_ram);
		RegisterModule(m_sifCmd);
	}
#ifdef _IOP_EMULATE_MODULES
	{
		m_fileIo = new Iop::CFileIo(*m_sifMan, *m_ioman);
		RegisterModule(m_fileIo);
	}
	{
		m_cdvdfsv = new Iop::CCdvdfsv(*m_sifMan, m_ram);
		RegisterModule(m_cdvdfsv);
	}
	{
		RegisterModule(new Iop::CMcServ(*m_sifMan));
	}
	{
		m_padman = new Iop::CPadMan(*m_sifMan);
		RegisterModule(m_padman);
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

uint32& CIopBios::CurrentThreadId() const
{
	return *reinterpret_cast<uint32*>(m_ram + BIOS_CURRENT_THREAD_ID_BASE);
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
		for(DynamicIopModuleListType::iterator moduleIterator(m_dynamicModules.begin());
			moduleIterator != m_dynamicModules.end(); moduleIterator++)
		{
			Iop::CDynamic* module(*moduleIterator);
			CStructFile moduleStruct;
			{
				uint32 importTableAddress = reinterpret_cast<uint8*>(module->GetExportTable()) - m_ram;
				moduleStruct.SetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS, importTableAddress); 
			}
			modulesFile->InsertStruct(module->GetId().c_str(), moduleStruct);
		}
	}
	archive.InsertFile(modulesFile);

	m_sifCmd->SaveState(archive);
	m_cdvdman->SaveState(archive);
}

void CIopBios::LoadState(Framework::CZipArchiveReader& archive)
{
	ClearDynamicModules();

	CStructCollectionStateFile modulesFile(*archive.BeginReadFile(STATE_MODULES));
	{
		for(CStructCollectionStateFile::StructIterator structIterator(modulesFile.GetStructBegin());
			structIterator != modulesFile.GetStructEnd(); structIterator++)
		{
			const CStructFile& structFile(structIterator->second);
			uint32 importTableAddress = structFile.GetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS);
			Iop::CDynamic* module = new Iop::CDynamic(reinterpret_cast<uint32*>(m_ram + importTableAddress));
			RegisterDynamicModule(module);
		}
	}

	m_sifCmd->LoadState(archive);
	m_cdvdman->LoadState(archive);

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

	m_moduleStarterThreadId = CreateThread(m_moduleStarterThreadProcAddress, DEFAULT_PRIORITY, DEFAULT_STACKSIZE, 0);
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

	moduleStartRequest->moduleId	= moduleId;
	moduleStartRequest->stopRequest	= stopRequest;

	assert((strlen(path) + 1) <= MODULESTARTREQUEST::MAX_PATH_SIZE);
	strncpy(moduleStartRequest->path, path, MODULESTARTREQUEST::MAX_PATH_SIZE);
	moduleStartRequest->path[MODULESTARTREQUEST::MAX_PATH_SIZE - 1] = 0;

	memcpy(moduleStartRequest->args, args, argsLength);
	moduleStartRequest->argsLength	= argsLength;

	WakeupThread(m_moduleStarterThreadId, false);
}

void CIopBios::ProcessModuleStart()
{
	static const auto pushToStack =
		[] (uint8* dst, uint32& stackAddress, const uint8* src, uint32 size)
		{
			uint32 fixedSize = ((size + 0x3) & ~0x3);
			uint32 copyAddress = stackAddress - size;
			stackAddress -= fixedSize;
			memcpy(dst + copyAddress, src, size);
			return copyAddress;
		};

	assert(GetCurrentThreadId() == m_moduleStarterThreadId);

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

	assert(GetCurrentThreadId() == m_moduleStarterThreadId);

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
		assert(moduleResidentState != MODULE_RESIDENT_STATE::NO_RESIDENT_END);
		assert(loadedModule->state == MODULE_STATE::STOPPED);
		loadedModule->state			= MODULE_STATE::STARTED;
		loadedModule->residentState	= moduleResidentState;

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
	uint32 handle = m_ioman->Open(Iop::Ioman::CDevice::OPEN_FLAG_RDONLY, path);
	if(handle & 0x80000000)
	{
		CLog::GetInstance().Print(LOGNAME, "Tried to load '%s' which couldn't be found.\r\n", path);
		return -1;
	}
	Iop::CIoman::CFile file(handle, *m_ioman);
	Framework::CStream* stream = m_ioman->GetFileStream(file);
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
	loadedModule->start			= moduleRange.first;
	loadedModule->entryPoint	= entryPoint;
	loadedModule->gp			= iopMod ? (iopMod->gp + moduleRange.first) : 0;
	loadedModule->state			= MODULE_STATE::STOPPED;

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
	struct IMAGE_FILE_PATTERN
	{
		const char* start;
		const char* pattern;
	};
	static const IMAGE_FILE_PATTERN g_imageFilePatterns[] =
	{
		{	"IOPRP",	"IOPRP%d.IMG;1"		},
		{	"DNAS",		"DNAS%d.IMG;1"		}
	};

	unsigned int imageVersion = 1000;
	for(const auto imageFilePattern : g_imageFilePatterns)
	{
		auto imageFileName = strstr(imagePath.c_str(), imageFilePattern.start);
		if(imageFileName != nullptr)
		{
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
				break;
			}
		}
	}
#ifdef _IOP_EMULATE_MODULES
	m_fileIo->SetModuleVersion(imageVersion);
#endif
}

CIopBios::THREAD* CIopBios::GetThread(uint32 threadId)
{
	return m_threads[threadId];
}

uint32 CIopBios::CreateThread(uint32 threadProc, uint32 priority, uint32 stackSize, uint32 optionData)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: CreateThread(threadProc = 0x%0.8X, priority = %d, stackSize = 0x%0.8X);\r\n", 
		CurrentThreadId(), threadProc, priority, stackSize);
#endif

	if(stackSize == 0)
	{
		stackSize = DEFAULT_STACKSIZE;
	}

	uint32 threadId = m_threads.Allocate();
	assert(threadId != -1);
	if(threadId == -1)
	{
		return -1;
	}

	THREAD* thread = m_threads[threadId];
	memset(&thread->context, 0, sizeof(thread->context));
	thread->context.delayJump = 1;
	thread->stackSize = stackSize;
	thread->stackBase = m_sysmem->AllocateMemory(thread->stackSize, 0, 0);
	memset(m_ram + thread->stackBase, 0, thread->stackSize);
	thread->id = threadId;
	thread->priority = priority;
	thread->initPriority = priority;
	thread->status = THREAD_STATUS_DORMANT;
	thread->threadProc = threadProc;
	thread->optionData = optionData;
	thread->nextActivateTime = 0;
	thread->context.gpr[CMIPS::GP] = m_cpu.m_State.nGPR[CMIPS::GP].nV0;
	thread->context.gpr[CMIPS::SP] = thread->stackBase + thread->stackSize - STACK_FRAME_RESERVE_SIZE;
	LinkThread(thread->id);
	return thread->id;
}

void CIopBios::DeleteThread(uint32 threadId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: DeleteThread(threadId = %d);\r\n", 
		CurrentThreadId(), threadId);
#endif
	THREAD* thread = m_threads[threadId];
	UnlinkThread(threadId);
	m_sysmem->FreeMemory(thread->stackBase);
	m_threads.Free(threadId);
}

int32 CIopBios::StartThread(uint32 threadId, uint32 param)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: StartThread(threadId = %i, param = 0x%0.8X);\r\n", 
		CurrentThreadId(), threadId, param);
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
			CurrentThreadId(), threadId);
		assert(false);
		return -1;
	}

	thread->status = THREAD_STATUS_RUNNING;
	thread->priority = thread->initPriority;
	thread->context.epc = thread->threadProc;
	thread->context.gpr[CMIPS::A0] = param;
	thread->context.gpr[CMIPS::RA] = m_threadFinishAddress;
	thread->context.gpr[CMIPS::SP] = thread->stackBase + thread->stackSize - STACK_FRAME_RESERVE_SIZE;

	// If the thread we are starting is the same priority or lower than the current one, do yield.
	// If may be that the correct action is never to yield - the docs aren't really clear.
	// INET.IRX (from Champions: Return to Arms) depends on startThread not yielding when starting a 
	// thread of the same priority.
	auto currentThread = GetThread(CurrentThreadId());
	if((currentThread == nullptr) || (currentThread->priority < thread->priority))
	{
		m_rescheduleNeeded = true;
	}

	return 0;
}

void CIopBios::ExitThread()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: ExitThread();\r\n", CurrentThreadId());
#endif
	THREAD* thread = GetThread(CurrentThreadId());
	thread->status = THREAD_STATUS_DORMANT;
	m_rescheduleNeeded = true;
}

uint32 CIopBios::TerminateThread(uint32 threadId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: TerminateThread(threadId = %d);\r\n", 
		CurrentThreadId(), threadId);
#endif

	assert(threadId != CurrentThreadId());
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
	return 0;
}

void CIopBios::DelayThread(uint32 delay)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: DelayThread(delay = %i);\r\n", 
		CurrentThreadId(), delay);
#endif

	THREAD* thread = GetThread(CurrentThreadId());
	thread->nextActivateTime = GetCurrentTime() + MicroSecToClock(delay);
	m_rescheduleNeeded = true;
}

void CIopBios::DelayThreadTicks(uint32 delay)
{
	auto thread = GetThread(CurrentThreadId());
	thread->nextActivateTime = GetCurrentTime() + delay;
	m_rescheduleNeeded = true;
}

uint32 CIopBios::SetAlarm(uint32 timePtr, uint32 alarmFunction, uint32 param)
{
	uint32 alarmThreadId = -1;

	//Find a thread we could recycle for a new alarm
	for(auto threadIterator = m_threads.Begin();
		threadIterator != m_threads.End(); threadIterator++)
	{
		const auto& thread(m_threads[threadIterator]);
		if(thread == nullptr) continue;
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
		alarmThreadId = CreateThread(m_alarmThreadProcAddress, 1, DEFAULT_STACKSIZE, 0);
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

	for(auto threadIterator = m_threads.Begin();
		threadIterator != m_threads.End(); threadIterator++)
	{
		const auto& thread(m_threads[threadIterator]);
		if(thread == nullptr) continue;
		if(thread->threadProc == m_alarmThreadProcAddress)
		{
			alarmThreadId = thread->id;
			break;
		}
	}

	if(alarmThreadId == -1)
	{
		// handler not registered
		return -105;
	}

	TerminateThread(alarmThreadId);
	return 0;
}

void CIopBios::ChangeThreadPriority(uint32 threadId, uint32 newPrio)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ChangeThreadPriority(threadId = %d, newPrio = %d);\r\n", 
		CurrentThreadId(), threadId, newPrio);
#endif

	if(threadId == 0)
	{
		threadId = GetCurrentThreadId();
	}

	THREAD* thread = GetThread(threadId);
	assert(thread != NULL);

	if(thread == NULL)
	{
		return;
	}

	thread->priority = newPrio;
	m_rescheduleNeeded = true;
}

uint32 CIopBios::ReferThreadStatus(uint32 threadId, uint32 statusPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReferThreadStatus(threadId = %d, statusPtr = 0x%0.8X);\r\n", 
		CurrentThreadId(), threadId, statusPtr);
#endif

	if(threadId == 0)
	{
		threadId = GetCurrentThreadId();
	}

	THREAD* thread = GetThread(threadId);
	assert(thread != NULL);

	if(thread == NULL)
	{
		return -1;
	}

	uint32 threadStatus = 0;
	if(thread->status == THREAD_STATUS_DORMANT)
	{
		threadStatus |= 0x10;
	}

	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_ATTRIBUTE]		= 0;
	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_OPTION]		= thread->optionData;
	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_STATUS]		= threadStatus;
	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_THREAD]		= thread->threadProc;
	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_STACK]			= thread->stackBase;
	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_STACKSIZE]		= thread->stackSize;
	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_INITPRIORITY]	= thread->initPriority;
	reinterpret_cast<uint32*>(m_ram + statusPtr)[THREAD_INFO_PRIORITY]		= thread->priority;

	return 0;
}

void CIopBios::SleepThread()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: SleepThread();\r\n", 
		CurrentThreadId());
#endif

	THREAD* thread = GetThread(CurrentThreadId());
	if(thread->status != THREAD_STATUS_RUNNING)
	{
		throw std::runtime_error("Thread isn't running.");
	}
	if(thread->wakeupCount == 0)
	{
		thread->status = THREAD_STATUS_SLEEPING;
		m_rescheduleNeeded = true;
	}
	else
	{
		thread->wakeupCount--;
	}
}

uint32 CIopBios::WakeupThread(uint32 threadId, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: WakeupThread(threadId = %d);\r\n", 
		CurrentThreadId(), threadId);
#endif

	THREAD* thread = GetThread(threadId);
	if(thread->status == THREAD_STATUS_SLEEPING)
	{
		thread->status = THREAD_STATUS_RUNNING;
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

void CIopBios::SleepThreadTillVBlankStart()
{
	THREAD* thread = GetThread(CurrentThreadId());
	thread->status = THREAD_STATUS_WAIT_VBLANK_START;
	m_rescheduleNeeded = true;
}

void CIopBios::SleepThreadTillVBlankEnd()
{
	THREAD* thread = GetThread(CurrentThreadId());
	thread->status = THREAD_STATUS_WAIT_VBLANK_END;
	m_rescheduleNeeded = true;
}

uint32 CIopBios::GetCurrentThreadId() const
{
	return CurrentThreadId();
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
	THREAD* thread = m_threads[threadId];
	uint32* nextThreadId = &ThreadLinkHead();
	while(1)
	{
		if((*nextThreadId) == 0)
		{
			(*nextThreadId) = threadId;
			thread->nextThreadId = 0;
			break;
		}
		THREAD* currentThread = m_threads[(*nextThreadId)];
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

	if(CurrentThreadId() != -1)
	{
		SaveThreadContext(CurrentThreadId());
		UnlinkThread(CurrentThreadId());
		LinkThread(CurrentThreadId());
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
	if(nextThreadId != CurrentThreadId())
	{
		CLog::GetInstance().Print(LOGNAME, "Switched over to thread %i.\r\n", nextThreadId);
	}
#endif
	CurrentThreadId() = nextThreadId;
}

uint32 CIopBios::GetNextReadyThread()
{
	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
		THREAD* nextThread = m_threads[nextThreadId];
		nextThreadId = nextThread->nextThreadId;
		if(GetCurrentTime() <= nextThread->nextActivateTime) continue;
		if(nextThread->status == THREAD_STATUS_RUNNING)
		{
			return nextThread->id;
		}
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
	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
		THREAD* nextThread = m_threads[nextThreadId];
		nextThreadId = nextThread->nextThreadId;
		if(nextThread->status == THREAD_STATUS_WAIT_VBLANK_START)
		{
			nextThread->status = THREAD_STATUS_RUNNING;
		}
	}
}

void CIopBios::NotifyVBlankEnd()
{
	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
		THREAD* nextThread = m_threads[nextThreadId];
		nextThreadId = nextThread->nextThreadId;
		if(nextThread->status == THREAD_STATUS_WAIT_VBLANK_END)
		{
			nextThread->status = THREAD_STATUS_RUNNING;
		}
	}
#ifdef _IOP_EMULATE_MODULES
	m_cdvdfsv->ProcessCommands(m_sifMan);
#endif
}

uint32 CIopBios::CreateSemaphore(uint32 initialCount, uint32 maxCount)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: CreateSemaphore(initialCount = %i, maxCount = %i);\r\n", 
		CurrentThreadId(), initialCount, maxCount);
#endif

	uint32 semaphoreId = m_semaphores.Allocate();
	assert(semaphoreId != -1);
	if(semaphoreId == -1)
	{
		return -1;
	}

	SEMAPHORE* semaphore = m_semaphores[semaphoreId];

	semaphore->count		= initialCount;
	semaphore->maxCount		= maxCount;
	semaphore->id			= semaphoreId;
	semaphore->waitCount	= 0;

	return semaphore->id;
}

uint32 CIopBios::DeleteSemaphore(uint32 semaphoreId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%i: DeleteSemaphore(semaphoreId = %i);\r\n",
		CurrentThreadId(), semaphoreId);
#endif

	SEMAPHORE* semaphore = m_semaphores[semaphoreId];
	if(semaphore == NULL)
	{
		CLog::GetInstance().Print(LOGNAME, "%i: Warning, trying to access invalid semaphore with id %i.\r\n",
			CurrentThreadId(), semaphoreId);
		return -1;
	}

	assert(semaphore->waitCount == 0);
	m_semaphores.Free(semaphoreId);

	return 0;
}

uint32 CIopBios::SignalSemaphore(uint32 semaphoreId, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: SignalSemaphore(semaphoreId = %d, inInterrupt = %d);\r\n", 
		CurrentThreadId(), semaphoreId, inInterrupt);
#endif

	SEMAPHORE* semaphore = m_semaphores[semaphoreId];
	if(semaphore == NULL)
	{
		CLog::GetInstance().Print(LOGNAME, "%d: Warning, trying to access invalid semaphore with id %d.\r\n",
			CurrentThreadId(), semaphoreId);
		return -1;
	}

	if(semaphore->waitCount != 0)
	{
		for(ThreadList::iterator threadIterator(m_threads.Begin());
			threadIterator != m_threads.End(); threadIterator++)
		{
			THREAD* thread(m_threads[threadIterator]);
			if(thread == NULL) continue;
			if(thread->waitSemaphore == semaphoreId)
			{
				if(thread->status != THREAD_STATUS_WAITING_SEMAPHORE)
				{
					throw std::runtime_error("Thread not waiting for semaphone (inconsistent state).");
				}
				thread->status = THREAD_STATUS_RUNNING;
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
	return semaphore->count;
}

uint32 CIopBios::WaitSemaphore(uint32 semaphoreId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: WaitSemaphore(semaphoreId = %d);\r\n", 
		CurrentThreadId(), semaphoreId);
#endif

	SEMAPHORE* semaphore = m_semaphores[semaphoreId];
	if(semaphore == NULL)
	{
		CLog::GetInstance().Print(LOGNAME, "%d: Warning, trying to access invalid semaphore with id %d.\r\n",
			CurrentThreadId(), semaphoreId);
		return -1;
	}

	if(semaphore->count == 0)
	{
		THREAD* thread = GetThread(CurrentThreadId());
		thread->status			= THREAD_STATUS_WAITING_SEMAPHORE;
		thread->waitSemaphore	= semaphoreId;
		semaphore->waitCount++;
		m_rescheduleNeeded = true;
	}
	else
	{
		semaphore->count--;
	}
	return semaphore->count;
}

uint32 CIopBios::PollSemaphore(uint32 semaphoreId)
{
	CLog::GetInstance().Print(LOGNAME, "%d: PollSemaphore(semaphoreId = %d);\r\n", 
		CurrentThreadId(), semaphoreId);

	auto semaphore = m_semaphores[semaphoreId];
	if(semaphore == nullptr)
	{
		return -1;
	}

	if(semaphore->count == 0)
	{
		return ERROR_SEMAPHORE_ZERO;
	}

	semaphore->count--;

	return 0;
}

uint32 CIopBios::ReferSemaphoreStatus(uint32 semaphoreId, uint32 statusPtr)
{
	CLog::GetInstance().Print(LOGNAME, "%d: ReferSemaphoreStatus(semaphoreId = %d, statusPtr = 0x%0.8X);\r\n", 
		CurrentThreadId(), semaphoreId, statusPtr);

	auto semaphore = m_semaphores[semaphoreId];
	if(semaphore == nullptr)
	{
		return -1;
	}

	auto status = reinterpret_cast<SEMAPHORE_STATUS*>(m_ram + statusPtr);
	status->attrib			= 0;
	status->option			= 0;
	status->initCount		= 0;
	status->maxCount		= semaphore->maxCount;
	status->currentCount	= semaphore->count;
	status->numWaitThreads	= semaphore->waitCount;

	return 0;
}

uint32 CIopBios::CreateEventFlag(uint32 attributes, uint32 options, uint32 initValue)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: CreateEventFlag(attr = 0x%0.8X, opt = 0x%0.8X, initValue = 0x%0.8X);\r\n",
		CurrentThreadId(), attributes, options, initValue);
#endif

	uint32 eventId = m_eventFlags.Allocate();
	assert(eventId != -1);
	if(eventId == -1)
	{
		return -1;
	}

	EVENTFLAG* eventFlag = m_eventFlags[eventId];

	eventFlag->id			= eventId;
	eventFlag->value		= initValue;
	eventFlag->options		= options;
	eventFlag->attributes	= attributes;

	return eventFlag->id;
}

uint32 CIopBios::SetEventFlag(uint32 eventId, uint32 value, bool inInterrupt)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: SetEventFlag(eventId = %d, value = 0x%0.8X, inInterrupt = %d);\r\n",
		CurrentThreadId(), eventId, value, inInterrupt);
#endif

	auto eventFlag = m_eventFlags[eventId];
	if(eventFlag == nullptr)
	{
		return -1;
	}

	eventFlag->value |= value;

	//Check all threads waiting for this event
	for(auto threadIterator(m_threads.Begin()); threadIterator != m_threads.End(); threadIterator++)
	{
		THREAD* thread(m_threads[threadIterator]);
		if(thread == NULL) continue;
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
	CLog::GetInstance().Print(LOGNAME, "%d: ClearEventFlag(eventId = %d, value = 0x%0.8X);\r\n",
		CurrentThreadId(), eventId, value);
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
	CLog::GetInstance().Print(LOGNAME, "%d: WaitEventFlag(eventId = %d, value = 0x%0.8X, mode = 0x%0.2X, resultPtr = 0x%0.8X);\r\n",
		CurrentThreadId(), eventId, value, mode, resultPtr);
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
		auto thread = GetThread(CurrentThreadId());
		thread->status					= THREAD_STATUS_WAITING_EVENTFLAG;
		thread->waitEventFlag			= eventId;
		thread->waitEventFlagMode		= mode;
		thread->waitEventFlagMask		= value;
		thread->waitEventFlagResultPtr	= resultPtr;
		m_rescheduleNeeded = true;
	}

	return 0;
}

uint32 CIopBios::ReferEventFlagStatus(uint32 eventId, uint32 infoPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReferEventFlagStatus(eventId = %d, infoPtr = 0x%0.8X);\r\n",
		CurrentThreadId(), eventId, infoPtr);
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
	eventFlagInfo->attributes	= eventFlag->attributes;
	eventFlagInfo->options		= eventFlag->options;
	eventFlagInfo->initBits		= 0;
	eventFlagInfo->currBits		= eventFlag->value;
	eventFlagInfo->numThreads	= 0;

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
		if(mode & WEF_CLEAR)
		{
			value &= ~mask;
		}

		if(resultPtr)
		{
			*resultPtr = maskResult;
		}
	}

	return success;
}

uint32 CIopBios::CreateMessageBox()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: CreateMessageBox();\r\n",
		CurrentThreadId());
#endif

	uint32 boxId = m_messageBoxes.Allocate();
	assert(boxId != -1);
	if(boxId == -1)
	{
		return -1;
	}

	MESSAGEBOX* box = m_messageBoxes[boxId];
	box->nextMsgPtr = 0;

	return boxId;
}

uint32 CIopBios::SendMessageBox(uint32 boxId, uint32 messagePtr)
{
	bool inInterrupt = false;

#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: SendMessageBox(boxId = %d, messagePtr = 0x%0.8X, inInterrupt = %d);\r\n",
		CurrentThreadId(), boxId, messagePtr, inInterrupt);
#endif

	MESSAGEBOX* box = m_messageBoxes[boxId];
	if(box == NULL)
	{
		return -1;
	}

	//Check if there's a thread waiting for a message first
	for(auto threadIterator(m_threads.Begin()); threadIterator != m_threads.End(); threadIterator++)
	{
		THREAD* thread(m_threads[threadIterator]);
		if(thread == NULL) continue;
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
			if(!inInterrupt)
			{
				m_rescheduleNeeded = true;
			}
			
			return 0;
		}
	}

	assert(0);
	return 0;
}

uint32 CIopBios::ReceiveMessageBox(uint32 messagePtr, uint32 boxId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: ReceiveMessageBox(messagePtr = 0x%0.8X, boxId = %d);\r\n",
		CurrentThreadId(), messagePtr, boxId);
#endif

	MESSAGEBOX* box = m_messageBoxes[boxId];
	if(box == NULL)
	{
		return -1;
	}

	if(box->nextMsgPtr != 0)
	{
		assert(0);
	}
	else
	{
		THREAD* thread = GetThread(CurrentThreadId());
		thread->status					= THREAD_STATUS_WAITING_MESSAGEBOX;
		thread->waitMessageBox			= boxId;
		thread->waitMessageBoxResultPtr	= messagePtr;
		m_rescheduleNeeded = true;
	}

	return 0;
}

uint32 CIopBios::PollMessageBox(uint32 messagePtr, uint32 boxId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "%d: PollMessageBox(messagePtr = 0x%0.8X, boxId = %d);\r\n",
		CurrentThreadId(), messagePtr, boxId);
#endif

	MESSAGEBOX* box = m_messageBoxes[boxId];
	if(box == NULL)
	{
		return -1;
	}

	if(box->nextMsgPtr == 0)
	{
		return 0xFFFFFE58;
	}

	assert(0);
	return 0;
}

Iop::CIoman* CIopBios::GetIoman()
{
	return m_ioman;
}

Iop::CCdvdman* CIopBios::GetCdvdman()
{
	return m_cdvdman;
}

Iop::CLoadcore* CIopBios::GetLoadcore()
{
	return m_loadcore;
}

#ifdef _IOP_EMULATE_MODULES

Iop::CPadMan* CIopBios::GetPadman()
{
	return m_padman;
}

Iop::CCdvdfsv* CIopBios::GetCdvdfsv()
{
	return m_cdvdfsv;
}

#endif

bool CIopBios::RegisterIntrHandler(uint32 line, uint32 mode, uint32 handler, uint32 arg)
{
	assert(FindIntrHandler(line) == -1);

	uint32 handlerId = m_intrHandlers.Allocate();
	assert(handlerId != -1);
	if(handlerId == -1)
	{
		return false;
	}

	INTRHANDLER* intrHandler = m_intrHandlers[handlerId];
	intrHandler->line		= line;
	intrHandler->mode		= mode;
	intrHandler->handler	= handler;
	intrHandler->arg		= arg;

	return true;
}

bool CIopBios::ReleaseIntrHandler(uint32 line)
{
	uint32 handlerId = FindIntrHandler(line);
	if(handlerId == -1)
	{
		return false;
	}
	m_intrHandlers.Free(handlerId);
	return true;
}

uint32 CIopBios::FindIntrHandler(uint32 line)
{
	for(IntrHandlerList::iterator handlerIterator(m_intrHandlers.Begin());
		handlerIterator != m_intrHandlers.End(); handlerIterator++)
	{
		INTRHANDLER* handler = m_intrHandlers[handlerIterator];
		if(handler == NULL) continue;
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

	assembler.MOV(CMIPS::S0, CMIPS::A0);		//S0 has the info struct ptr

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

		IopModuleMapType::iterator module(m_modules.find(moduleName));
		if(module != m_modules.end())
		{
			module->second->Invoke(m_cpu, functionId);
		}
		else
		{
#ifdef _DEBUG
			CLog::GetInstance().Print(LOGNAME, "%0.8X: Trying to call a function from non-existing module (%s, %d).\r\n", 
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
				if(CurrentThreadId() != -1)
				{
					SaveThreadContext(CurrentThreadId());
				}
				CurrentThreadId() = -1;
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
	while(m_modules.size() != 0)
	{
		delete m_modules.begin()->second;
		m_modules.erase(m_modules.begin());
	}
	m_dynamicModules.clear();

	m_libsd.reset();

	m_sifMan = NULL;
	m_stdio = NULL;
	m_ioman = NULL;
	m_sysmem = NULL;
	m_modload = NULL;
	m_sysmem = NULL;
#ifdef _IOP_EMULATE_MODULES
	m_padman = NULL;
	m_cdvdfsv = NULL;
#endif
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

void CIopBios::RegisterModule(Iop::CModule* module)
{
	m_modules[module->GetId()] = module;
}

void CIopBios::ClearDynamicModules()
{
	for(DynamicIopModuleListType::iterator dynModuleIterator(m_dynamicModules.begin());
		dynModuleIterator != m_dynamicModules.end(); dynModuleIterator++)
	{
		IopModuleMapType::iterator module(m_modules.find((*dynModuleIterator)->GetId()));
		assert(module != m_modules.end());
		if(module != m_modules.end())
		{
			delete module->second;
			m_modules.erase(module);
		}
	}

	m_dynamicModules.clear();
}

void CIopBios::RegisterDynamicModule(Iop::CDynamic* dynamicModule)
{
	m_dynamicModules.push_back(dynamicModule);
	RegisterModule(dynamicModule);
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
	//Process relocation
	const ELFHEADER& header = elf.GetHeader();
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
		ELFSECTIONHEADER* sectionHeader = elf.GetSection(i);
		if(sectionHeader != NULL && sectionHeader->nType == CELF::SHT_REL)
		{
			uint32 lastHi16 = -1;
			uint32 instructionHi16 = -1;
			unsigned int linkedSection = sectionHeader->nInfo;
			unsigned int recordCount = sectionHeader->nSize / 8;
			ELFSECTIONHEADER* relocatedSection = elf.GetSection(linkedSection);
			const uint32* relocationRecord = reinterpret_cast<const uint32*>(elf.GetSectionData(i));
			uint8* relocatedSectionData = reinterpret_cast<uint8*>(const_cast<void*>(elf.GetSectionData(linkedSection)));
			if(relocatedSection == NULL || relocationRecord == NULL || relocatedSectionData == NULL) continue;
			uint32 sectionBase = relocatedSection->nStart;
			for(unsigned int record = 0; record < recordCount; record++)
			{
				uint32 relocationAddress = relocationRecord[0] - sectionBase;
				uint32 relocationType = relocationRecord[1] & 0xFF;
				if(relocationAddress < relocatedSection->nSize) 
				{
					uint32& instruction = *reinterpret_cast<uint32*>(&relocatedSectionData[relocationAddress]);
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
						{
							lastHi16 = relocationAddress;
							instructionHi16 = instruction;
						}
						break;
					case CELF::R_MIPS_LO16:
						{
							if(lastHi16 != -1)
							{
								uint32 offset = static_cast<int16>(instruction) + (instructionHi16 << 16);
								offset += baseAddress;
								instruction &= ~0xFFFF;
								instruction |= offset & 0xFFFF;

								uint32& prevInstruction = *reinterpret_cast<uint32*>(&relocatedSectionData[lastHi16]);
								prevInstruction &= ~0xFFFF;
								if(offset & 0x8000) offset += 0x10000;
								prevInstruction |= offset >> 16;
								lastHi16 = -1;
							}
							else
							{
#ifdef _DEBUG
								CLog::GetInstance().Print(LOGNAME, "%s: No HI16 relocation record found for corresponding LO16.\r\n", 
									__FUNCTION__);
#endif
							}
						}
						break;
					default:
						throw std::runtime_error("Unknown relocation type.");
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
	for (auto threadIterator = m_threads.Begin();
		threadIterator != m_threads.End(); threadIterator++)
	{
		const auto& thread(m_threads[threadIterator]);
		if(thread == nullptr) continue;
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
		callbackThreadId = CreateThread(address, DEFAULT_PRIORITY, DEFAULT_STACKSIZE, 0);
	}

	ChangeThreadPriority(callbackThreadId, 1);
	StartThread(callbackThreadId, 0);

	auto thread = GetThread(callbackThreadId);
	thread->context.gpr[CMIPS::A0] = arg0;
	thread->context.gpr[CMIPS::A1] = arg1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Debug Stuff

#ifdef DEBUGGER_INCLUDED

#define TAGS_SECTION_IOP_MODULES						("modules")
#define TAGS_SECTION_IOP_MODULES_MODULE					("module")
#define TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS	("beginAddress")
#define TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS		("endAddress")
#define TAGS_SECTION_IOP_MODULES_MODULE_NAME			("name")

void CIopBios::LoadDebugTags(Framework::Xml::CNode* root)
{
	auto moduleSection = root->Select(TAGS_SECTION_IOP_MODULES);
	if(moduleSection == NULL) return;

	for(Framework::Xml::CFilteringNodeIterator nodeIterator(moduleSection, TAGS_SECTION_IOP_MODULES_MODULE);
		!nodeIterator.IsEnd(); nodeIterator++)
	{
		auto moduleNode(*nodeIterator);
		const char* moduleName		= moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_NAME);
		const char* beginAddress	= moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS);
		const char* endAddress		= moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS);
		if(!moduleName || !beginAddress || !endAddress) continue;
		if(FindModuleDebugInfo(moduleName) != std::end(m_moduleTags)) continue;

		BIOS_DEBUG_MODULE_INFO module;
		module.name		= moduleName;
		module.begin	= lexical_cast_hex<std::string>(beginAddress);
		module.end		= lexical_cast_hex<std::string>(endAddress);
		module.param	= NULL;
		m_moduleTags.push_back(module);
	}
}

void CIopBios::SaveDebugTags(Framework::Xml::CNode* root)
{
	Framework::Xml::CNode* moduleSection = new Framework::Xml::CNode(TAGS_SECTION_IOP_MODULES, true);

	for(auto moduleIterator(std::begin(m_moduleTags));
		std::end(m_moduleTags) != moduleIterator; moduleIterator++)
	{
		const auto& module(*moduleIterator);
		Framework::Xml::CNode* moduleNode = new Framework::Xml::CNode(TAGS_SECTION_IOP_MODULES_MODULE, true);
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS,	lexical_cast_hex<std::string>(module.begin, 8).c_str());
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS,		lexical_cast_hex<std::string>(module.end, 8).c_str());
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_NAME,			module.name.c_str());
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

	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
		THREAD* nextThread = m_threads[nextThreadId];

		BIOS_DEBUG_THREAD_INFO threadInfo;
		threadInfo.id			= nextThreadId;
		threadInfo.priority		= nextThread->priority;
		if(GetCurrentThreadId() == nextThreadId)
		{
			threadInfo.pc = m_cpu.m_State.nPC;
			threadInfo.ra = m_cpu.m_State.nGPR[CMIPS::RA].nV0;
			threadInfo.sp = m_cpu.m_State.nGPR[CMIPS::SP].nV0;
		}
		else
		{
			threadInfo.pc = nextThread->context.epc;
			threadInfo.ra = nextThread->context.gpr[CMIPS::RA];
			threadInfo.sp = nextThread->context.gpr[CMIPS::SP];
		}

		switch(nextThread->status)
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
			threadInfo.stateDescription = "Waiting (Semaphore: " + boost::lexical_cast<std::string>(nextThread->waitSemaphore) + ")";
			break;
		case THREAD_STATUS_WAITING_EVENTFLAG:
			threadInfo.stateDescription = "Waiting (Event Flag: " + boost::lexical_cast<std::string>(nextThread->waitEventFlag) + ")";
			break;
		case THREAD_STATUS_WAITING_MESSAGEBOX:
			threadInfo.stateDescription = "Waiting (Message Box: " + boost::lexical_cast<std::string>(nextThread->waitMessageBox) + ")";
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

		nextThreadId = nextThread->nextThreadId;
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
		module.name		= moduleName;
		module.begin	= moduleRange.first;
		module.end		= moduleRange.second;
		module.param	= NULL;
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
					sprintf(functionNameTemp, "unknown_%0.4X", functionId);
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

	CLog::GetInstance().Print(LOGNAME, "Loaded IOP module '%s' @ 0x%0.8X.\r\n", 
		modulePath.c_str(), moduleRange.first);
}

BiosDebugModuleInfoIterator CIopBios::FindModuleDebugInfo(const std::string& name)
{
	return std::find_if(std::begin(m_moduleTags), std::end(m_moduleTags),
		[=] (const BIOS_DEBUG_MODULE_INFO& module) 
		{
			return name == module.name;
		}
	);
}

BiosDebugModuleInfoIterator CIopBios::FindModuleDebugInfo(uint32 beginAddress, uint32 endAddress)
{
	return std::find_if(std::begin(m_moduleTags), std::end(m_moduleTags),
		[=] (const BIOS_DEBUG_MODULE_INFO& module) 
		{
			return (beginAddress == module.begin) && (endAddress == module.end);
		}
	);
}

#endif
