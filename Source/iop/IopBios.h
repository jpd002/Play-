#pragma once

#include <memory>
#include <list>
#include <map>
#include <set>
#include "../MIPSAssembler.h"
#include "../MIPS.h"
#include "../ELF.h"
#include "../OsStructManager.h"
#include "../OsVariableWrapper.h"
#include "Iop_BiosBase.h"
#include "Iop_BiosStructs.h"
#include "Iop_SifMan.h"
#include "Iop_SifCmd.h"
#include "Iop_Ioman.h"
#include "Iop_Cdvdman.h"
#include "Iop_Stdio.h"
#include "Iop_Sysmem.h"
#include "Iop_Modload.h"
#include "Iop_Loadcore.h"
#include "Iop_LibSd.h"
#ifdef _IOP_EMULATE_MODULES
#include "Iop_FileIo.h"
#include "Iop_PadMan.h"
#include "Iop_MtapMan.h"
#include "Iop_Cdvdfsv.h"
#include "Iop_McServ.h"
#endif

class CIopBios : public Iop::CBiosBase
{
public:
	enum KERNEL_RESULT_CODES
	{
		KERNEL_RESULT_OK = 0,
		KERNEL_RESULT_ERROR = -1,
		KERNEL_RESULT_ERROR_ILLEGAL_CONTEXT = -100,
		KERNEL_RESULT_ERROR_ILLEGAL_INTRCODE = -101,
		KERNEL_RESULT_ERROR_FOUND_HANDLER = -104,
		KERNEL_RESULT_ERROR_NOTFOUND_HANDLER = -105,
		KERNEL_RESULT_ERROR_NO_TIMER = -150,
		KERNEL_RESULT_ERROR_ILLEGAL_TIMERID = -151,
		KERNEL_RESULT_ERROR_NO_MEMORY = -400,
		KERNEL_RESULT_ERROR_ILLEGAL_ATTR = -401,
		KERNEL_RESULT_ERROR_ILLEGAL_ENTRY = -402,
		KERNEL_RESULT_ERROR_ILLEGAL_PRIORITY = -403,
		KERNEL_RESULT_ERROR_ILLEGAL_THID = -406,
		KERNEL_RESULT_ERROR_UNKNOWN_THID = -407,
		KERNEL_RESULT_ERROR_UNKNOWN_SEMAID = -408,
		KERNEL_RESULT_ERROR_UNKNOWN_EVFID = -409,
		KERNEL_RESULT_ERROR_UNKNOWN_MBXID = -410,
		KERNEL_RESULT_ERROR_UNKNOWN_VPLID = -411,
		KERNEL_RESULT_ERROR_UNKNOWN_FPLID = -412,
		KERNEL_RESULT_ERROR_NOT_DORMANT = -414,
		KERNEL_RESULT_ERROR_NOT_WAIT = -416,
		KERNEL_RESULT_ERROR_RELEASE_WAIT = -418,
		KERNEL_RESULT_ERROR_SEMA_ZERO = -419,
		KERNEL_RESULT_ERROR_SEMA_OVF = -420,
		KERNEL_RESULT_ERROR_EVF_CONDITION = -421,
		KERNEL_RESULT_ERROR_EVF_ILLEGAL_PAT = -423,
		KERNEL_RESULT_ERROR_MBX_NOMSG = -424,
		KERNEL_RESULT_ERROR_WAIT_DELETE = -425,
		KERNEL_RESULT_ERROR_ILLEGAL_MEMBLOCK = -426,
		KERNEL_RESULT_ERROR_ILLEGAL_MEMSIZE = -427,
	};

	enum CONTROL_BLOCK
	{
		CONTROL_BLOCK_START = 0x100,
		CONTROL_BLOCK_END = 0x10000,
	};

	struct THREADCONTEXT
	{
		uint32 gpr[0x20];
		uint32 epc;
		uint32 delayJump;
	};

	struct THREAD
	{
		uint32 isValid;
		uint32 id;
		uint32 initPriority;
		uint32 priority;
		uint32 optionData;
		uint32 attributes;
		uint32 threadProc;
		THREADCONTEXT context;
		uint32 status;
		uint32 waitSemaphore;
		uint32 waitEventFlag;
		uint32 waitEventFlagMode;
		uint32 waitEventFlagMask;
		uint32 waitEventFlagResultPtr;
		uint32 waitMessageBox;
		uint32 waitMessageBoxResultPtr;
		uint32 wakeupCount;
		uint32 stackBase;
		uint32 stackSize;
		uint32 nextThreadId;
		uint64 nextActivateTime;
	};

	enum THREAD_STATUS
	{
		THREAD_STATUS_DORMANT = 1,
		THREAD_STATUS_RUNNING = 2,
		THREAD_STATUS_SLEEPING = 3,
		THREAD_STATUS_WAITING_SEMAPHORE = 4,
		THREAD_STATUS_WAITING_EVENTFLAG = 5,
		THREAD_STATUS_WAITING_MESSAGEBOX = 6,
		THREAD_STATUS_WAIT_VBLANK_START = 7,
		THREAD_STATUS_WAIT_VBLANK_END = 8,
		THREAD_STATUS_WAIT_CDSYNC = 9,
	};

	struct THREAD_INFO
	{
		uint32 attributes;
		uint32 option;
		uint32 status;
		uint32 entryPoint;
		uint32 stackAddr;
		uint32 stackSize;
		uint32 gp;
		uint32 initPriority;
		uint32 currentPriority;
		uint32 waitType;
		uint32 waitId;
		uint32 wakeupCount;
		uint32 regContextAddr;
		uint32 reserved[4];
	};

	CIopBios(CMIPS&, uint8*, uint32, uint8*);
	virtual ~CIopBios();

	int32 LoadModule(const char*);
	int32 LoadModule(uint32);
	int32 LoadModuleFromHost(uint8*);
	int32 UnloadModule(uint32);
	int32 StartModule(uint32, const char*, const char*, uint32);
	int32 StopModule(uint32);
	bool CanStopModule(uint32) const;
	bool IsModuleHle(uint32) const;
	int32 SearchModuleByName(const char*) const;
	void ProcessModuleReset(const std::string&);

	bool TryGetImageVersionFromPath(const std::string&, unsigned int*);
	bool TryGetImageVersionFromContents(const std::string&, unsigned int*);

	void HandleException() override;
	void HandleInterrupt() override;

	void Reschedule();

	void CountTicks(uint32) override;
	uint64 GetCurrentTime() const;
	uint64 MilliSecToClock(uint32);
	uint64 MicroSecToClock(uint32);
	uint64 ClockToMicroSec(uint64);

	void NotifyVBlankStart() override;
	void NotifyVBlankEnd() override;

	void Reset(const Iop::SifManPtr&);

	void SaveState(Framework::CZipArchiveWriter&) override;
	void LoadState(Framework::CZipArchiveReader&) override;

	bool IsIdle() override;

	Iop::CSysmem* GetSysmem();
	Iop::CIoman* GetIoman();
	Iop::CCdvdman* GetCdvdman();
	Iop::CLoadcore* GetLoadcore();
#ifdef _IOP_EMULATE_MODULES
	Iop::CPadMan* GetPadman();
	Iop::CCdvdfsv* GetCdvdfsv();
	Iop::CMcServ* GetMcServ();
#endif
	bool RegisterModule(const Iop::ModulePtr&);
	bool ReleaseModule(const std::string&);

	uint32 CreateThread(uint32, uint32, uint32, uint32, uint32);
	int32 DeleteThread(uint32);
	int32 StartThread(uint32, uint32);
	int32 StartThreadArgs(uint32, uint32, uint32);
	void ExitThread();
	uint32 TerminateThread(uint32);
	int32 DelayThread(uint32);
	void DelayThreadTicks(uint32);
	uint32 SetAlarm(uint32, uint32, uint32);
	uint32 CancelAlarm(uint32, uint32);
	THREAD* GetThread(uint32);
	int32 GetCurrentThreadId();
	int32 GetCurrentThreadIdRaw() const;
	int32 ChangeThreadPriority(uint32, uint32);
	uint32 ReferThreadStatus(uint32, uint32, bool);
	int32 SleepThread();
	uint32 WakeupThread(uint32, bool);
	int32 CancelWakeupThread(uint32, bool);
	int32 RotateThreadReadyQueue(uint32);
	int32 ReleaseWaitThread(uint32, bool);

	int32 RegisterVblankHandler(uint32, uint32, uint32, uint32);
	int32 ReleaseVblankHandler(uint32, uint32);
	int32 FindVblankHandlerByLineAndPtr(uint32 startEnd, uint32 handlerPtr);
	void SleepThreadTillVBlankStart();
	void SleepThreadTillVBlankEnd();

	uint32 CreateSemaphore(uint32, uint32);
	uint32 DeleteSemaphore(uint32);
	uint32 SignalSemaphore(uint32, bool);
	uint32 WaitSemaphore(uint32);
	uint32 PollSemaphore(uint32);
	uint32 ReferSemaphoreStatus(uint32, uint32);
	bool SemaReleaseSingleThread(uint32, bool);

	uint32 CreateEventFlag(uint32, uint32, uint32);
	uint32 DeleteEventFlag(uint32);
	uint32 SetEventFlag(uint32, uint32, bool);
	uint32 ClearEventFlag(uint32, uint32);
	uint32 WaitEventFlag(uint32, uint32, uint32, uint32);
	uint32 PollEventFlag(uint32, uint32, uint32, uint32);
	uint32 ReferEventFlagStatus(uint32, uint32);
	bool ProcessEventFlag(uint32, uint32&, uint32, uint32*);

	uint32 CreateMessageBox();
	uint32 DeleteMessageBox(uint32);
	uint32 SendMessageBox(uint32, uint32, bool);
	uint32 ReceiveMessageBox(uint32, uint32);
	uint32 PollMessageBox(uint32, uint32);
	uint32 ReferMessageBoxStatus(uint32, uint32);

	uint32 CreateFpl(uint32);
	uint32 AllocateFpl(uint32);
	uint32 pAllocateFpl(uint32);
	uint32 FreeFpl(uint32, uint32);

	uint32 CreateVpl(uint32);
	uint32 DeleteVpl(uint32);
	uint32 pAllocateVpl(uint32, uint32);
	uint32 FreeVpl(uint32, uint32);
	uint32 ReferVplStatus(uint32, uint32);
	uint32 GetVplFreeSize(uint32);

	void WaitCdSync();
	void ReleaseWaitCdSync();

	int32 RegisterIntrHandler(uint32, uint32, uint32, uint32);
	int32 ReleaseIntrHandler(uint32);

	void TriggerCallback(uint32 address, uint32 arg0 = 0, uint32 arg1 = 0, uint32 arg2 = 0, uint32 arg3 = 0);

#ifdef DEBUGGER_INCLUDED
	void LoadDebugTags(Framework::Xml::CNode*) override;
	void SaveDebugTags(Framework::Xml::CNode*) override;

	BiosDebugModuleInfoArray GetModulesDebugInfo() const override;
	BiosDebugThreadInfoArray GetThreadsDebugInfo() const override;
#endif

	typedef Framework::CSignal<void(const char*)> ModuleLoadedEvent;
	typedef Framework::CSignal<void(uint32)> ModuleStartedEvent;

	ModuleLoadedEvent OnModuleLoaded;
	ModuleStartedEvent OnModuleStarted;

private:
	enum DEFAULT_STACKSIZE
	{
		DEFAULT_STACKSIZE = 0x4000,
	};

	enum DEFAULT_PRIORITY
	{
		DEFAULT_PRIORITY = 64,
	};

	enum MODULE_INIT_PRIORITY
	{
		MODULE_INIT_PRIORITY = 8,
	};

	enum class MODULE_STATE : uint32
	{
		STOPPED,
		STARTED,
		HLE,
	};

	enum class MODULE_RESIDENT_STATE : uint32
	{
		RESIDENT_END = 0,
		NO_RESIDENT_END = 1,
		REMOVABLE_RESIDENT_END = 2,
	};

	enum
	{
		MAX_THREAD = 128,
		MAX_MEMORYBLOCK = 256,
		MAX_SEMAPHORE = 128,
		MAX_EVENTFLAG = 64,
		MAX_INTRHANDLER = 32,
		MAX_VBLANKHANDLER = 8,
		MAX_MESSAGEBOX = 32,
		MAX_FPL = 16,
		MAX_VPL = 16,
		MAX_MODULESTARTREQUEST = 32,
		MAX_LOADEDMODULE = 32,
	};

	enum WEF_FLAGS
	{
		WEF_AND = 0x00,
		WEF_OR = 0x01,
		WEF_CLEAR = 0x10,
	};

	struct SEMAPHORE
	{
		uint32 isValid;
		uint32 id;
		uint32 count;
		uint32 maxCount;
		uint32 waitCount;
	};

	struct SEMAPHORE_STATUS
	{
		uint32 attrib;
		uint32 option;
		uint32 initCount;
		uint32 maxCount;
		uint32 currentCount;
		uint32 numWaitThreads;
	};

	struct EVENTFLAG
	{
		uint32 isValid;
		uint32 id;
		uint32 attributes;
		uint32 options;
		uint32 value;
	};

	struct EVENTFLAGINFO
	{
		uint32 attributes;
		uint32 options;
		uint32 initBits;
		uint32 currBits;
		uint32 numThreads;
	};

	struct INTRHANDLER
	{
		uint32 isValid;
		uint32 line;
		uint32 mode;
		uint32 handler;
		uint32 arg;
	};

	struct VBLANKHANDLER
	{
		uint32 isValid;
		uint32 type;
		uint32 handler;
		uint32 arg;
	};
	static_assert(sizeof(VBLANKHANDLER) == 0x10, "Size of VBLANKHANDLER must be 16 bytes. AssembleVblankHandler relies on this.");

	struct MESSAGEBOX
	{
		uint32 isValid;
		uint32 nextMsgPtr;
		uint32 numMessage;
	};

	struct MESSAGEBOX_STATUS
	{
		uint32 attr;
		uint32 option;
		uint32 numWaitThread;
		uint32 numMessage;
		uint32 messagePtr;
		uint32 unused[2];
	};

	struct MESSAGE_HEADER
	{
		uint32 nextMsgPtr;
		uint8 priority;
		uint8 unused[3];
	};

	enum FPL_ATTR
	{
		FPL_ATTR_THFIFO = 0x000,
		FPL_ATTR_THPRI = 0x001,
		FPL_ATTR_THMODE_MASK = 0x001,
		FPL_ATTR_MEMBTM = 0x200,
		FPL_ATTR_MEMMODE_MASK = 0x200,
		FPL_ATTR_VALID_MASK = (FPL_ATTR_THMODE_MASK | FPL_ATTR_MEMMODE_MASK)
	};

	struct FPL_PARAM
	{
		uint32 attr;
		uint32 option;
		uint32 blockSize;
		uint32 blockCount;
	};

	struct FPL
	{
		uint32 isValid;
		uint32 attr;
		uint32 option;
		uint32 poolPtr;
		uint32 blockSize;
		uint32 blockCount;
	};

	struct VPL
	{
		uint32 isValid;
		uint32 attr;
		uint32 option;
		uint32 poolPtr;
		uint32 size;
		uint32 headBlockId;
	};

	enum VPL_ATTR
	{
		VPL_ATTR_THFIFO = 0x000,
		VPL_ATTR_THPRI = 0x001,
		VPL_ATTR_THMODE_MASK = 0x001,
		VPL_ATTR_MEMBTM = 0x200,
		VPL_ATTR_MEMMODE_MASK = 0x200,
		VPL_ATTR_VALID_MASK = (VPL_ATTR_THMODE_MASK | VPL_ATTR_MEMMODE_MASK)
	};

	struct VPL_PARAM
	{
		uint32 attr;
		uint32 option;
		uint32 size;
	};

	struct VPL_STATUS
	{
		uint32 attr;
		uint32 option;
		uint32 size;
		uint32 freeSize;
		uint32 numWaitThreads;
		uint32 unused[3];
	};

	struct MODULESTARTREQUEST
	{
		enum
		{
			MAX_PATH_SIZE = 256,
			MAX_ARGS_SIZE = 256
		};

		uint32 nextPtr;
		uint32 moduleId;
		uint32 stopRequest;
		char path[MAX_PATH_SIZE];
		uint32 argsLength;
		char args[MAX_ARGS_SIZE];
	};

	struct LOADEDMODULE
	{
		enum
		{
			MAX_NAME_SIZE = 0x100,
		};

		uint32 isValid;
		char name[MAX_NAME_SIZE];
		uint32 start;
		uint32 end;
		uint32 entryPoint;
		uint32 gp;
		MODULE_STATE state;
		MODULE_RESIDENT_STATE residentState;
	};

	struct IOPMOD
	{
		uint32 module;
		uint32 startAddress;
		uint32 gp;
		uint32 textSectionSize;
		uint32 dataSectionSize;
		uint32 bssSectionSize;
		uint16 moduleStructAttr;
		char moduleName[256];
	};

	enum
	{
		IOPMOD_SECTION_ID = 0x70000080,
	};

	enum
	{
		ET_SCE_IOPRELEXEC = 0xFF80,
		ET_SCE_IOPRELEXEC2 = 0xFF81
	};

	enum
	{
		R_MIPSSCE_MHI16 = 0xFA,
		R_MIPSSCE_ADDEND = 0xFB
	};

	struct SYSTEM_INTRHANDLER
	{
		uint32 handler;
		uint32 paramPtr;
	};
	static_assert(sizeof(SYSTEM_INTRHANDLER) == 0x8, "Size of SYSTEM_INTRHANDLER must be 8 bytes. Fixed PS2 structure, and we use it for array magic iteration.");

	typedef COsStructManager<THREAD> ThreadList;
	typedef COsStructManager<Iop::MEMORYBLOCK> MemoryBlockList;
	typedef COsStructManager<SEMAPHORE> SemaphoreList;
	typedef COsStructManager<EVENTFLAG> EventFlagList;
	typedef COsStructManager<INTRHANDLER> IntrHandlerList;
	typedef COsStructManager<VBLANKHANDLER> VblankHandlerList;
	typedef COsStructManager<MESSAGEBOX> MessageBoxList;
	typedef COsStructManager<FPL> FplList;
	typedef COsStructManager<VPL> VplList;
	typedef COsStructManager<LOADEDMODULE> LoadedModuleList;
	typedef std::map<std::string, Iop::ModulePtr> IopModuleMapType;
	typedef std::set<Iop::CModule*> ModuleSet;
	typedef std::pair<uint32, uint32> ExecutableRange;

	void LoadThreadContext(uint32);
	void SaveThreadContext(uint32);
	uint32 GetNextReadyThread();
	void ReturnFromException();

	uint32 FindIntrHandler(uint32);

	void LinkThread(uint32);
	void UnlinkThread(uint32);

	uint32& ThreadLinkHead() const;
	uint64& CurrentTime() const;
	uint32& ModuleStartRequestHead() const;
	uint32& ModuleStartRequestFree() const;

	int32 LoadModule(CELF&, const char*);
	uint32 LoadExecutable(CELF&, ExecutableRange&);
	unsigned int GetElfProgramToLoad(CELF&);
	void RelocateElf(CELF&, uint32);
	std::string ReadModuleName(uint32);
	void DeleteModules();

	int32 LoadHleModule(const Iop::ModulePtr&);
	void RegisterHleModule(const Iop::ModulePtr&);

	ModuleSet GetBuiltInModules() const;

	uint32 AssembleThreadFinish(CMIPSAssembler&);
	uint32 AssembleReturnFromException(CMIPSAssembler&);
	uint32 AssembleIdleFunction(CMIPSAssembler&);
	uint32 AssembleModuleStarterThreadProc(CMIPSAssembler&);
	uint32 AssembleAlarmThreadProc(CMIPSAssembler&);
	uint32 AssembleVblankHandler(CMIPSAssembler&);

	void InitializeModuleStarter();
	void ProcessModuleStart();
	void FinishModuleStart();
	void RequestModuleStart(bool, uint32, const char*, const char*, unsigned int);

	void PopulateSystemIntcHandlers();

#ifdef DEBUGGER_INCLUDED
	void PrepareModuleDebugInfo(CELF&, const ExecutableRange&, const std::string&, const std::string&);
	BiosDebugModuleInfoIterator FindModuleDebugInfo(const std::string&);
	BiosDebugModuleInfoIterator FindModuleDebugInfo(uint32, uint32);
#endif

	CMIPS& m_cpu;
	uint8* m_ram = nullptr;
	uint32 m_ramSize;
	uint8* m_spr = nullptr;
	uint32 m_threadFinishAddress;
	uint32 m_returnFromExceptionAddress;
	uint32 m_idleFunctionAddress;
	uint32 m_moduleStarterThreadProcAddress;
	uint32 m_alarmThreadProcAddress;
	uint32 m_vblankHandlerAddress;

	uint32 m_moduleStarterThreadId;

	bool m_rescheduleNeeded = false;
	LoadedModuleList m_loadedModules;
	ThreadList m_threads;
	MemoryBlockList m_memoryBlocks;
	SemaphoreList m_semaphores;
	EventFlagList m_eventFlags;
	IntrHandlerList m_intrHandlers;
	VblankHandlerList m_vblankHandlers;
	MessageBoxList m_messageBoxes;
	FplList m_fpls;
	VplList m_vpls;

	IopModuleMapType m_modules;

	OsVariableWrapper<uint32> m_currentThreadId;

#ifdef DEBUGGER_INCLUDED
	BiosDebugModuleInfoArray m_moduleTags;
#endif
	Iop::SifManPtr m_sifMan;
	Iop::SifCmdPtr m_sifCmd;
	Iop::StdioPtr m_stdio;
	Iop::IomanPtr m_ioman;
	Iop::CdvdmanPtr m_cdvdman;
	Iop::SysmemPtr m_sysmem;
	Iop::ModloadPtr m_modload;
	Iop::LoadcorePtr m_loadcore;
	Iop::ModulePtr m_libsd;
#ifdef _IOP_EMULATE_MODULES
	Iop::FileIoPtr m_fileIo;
	Iop::PadManPtr m_padman;
	Iop::MtapManPtr m_mtapman;
	Iop::McServPtr m_mcserv;
	Iop::CdvdfsvPtr m_cdvdfsv;

	std::map<std::string, Iop::ModulePtr> m_hleModules;
#endif
};

typedef std::shared_ptr<CIopBios> IopBiosPtr;
