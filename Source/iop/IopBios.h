#pragma once

#include <memory>
#include <list>
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
#endif

class CIopBios : public Iop::CBiosBase
{
public:
	enum CONTROL_BLOCK
	{
		CONTROL_BLOCK_START	= 0x10,
		CONTROL_BLOCK_END	= 0x10000,
	};

	struct THREADCONTEXT
	{
		uint32		gpr[0x20];
		uint32		epc;
		uint32		delayJump;
	};

	struct THREAD
	{
		uint32			isValid;
		uint32			id;
		uint32			initPriority;
		uint32			priority;
		uint32			optionData;
		uint32			threadProc;
		THREADCONTEXT	context;
		uint32			status;
		uint32			waitSemaphore;
		uint32			waitEventFlag;
		uint32			waitEventFlagMode;
		uint32			waitEventFlagMask;
		uint32			waitEventFlagResultPtr;
		uint32			waitMessageBox;
		uint32			waitMessageBoxResultPtr;
		uint32			wakeupCount;
		uint32			stackBase;
		uint32			stackSize;
		uint32			nextThreadId;
		uint64			nextActivateTime;
	};

	enum THREAD_STATUS
	{
		THREAD_STATUS_DORMANT				= 1,
		THREAD_STATUS_RUNNING				= 2,
		THREAD_STATUS_SLEEPING				= 3,
		THREAD_STATUS_WAITING_SEMAPHORE		= 4,
		THREAD_STATUS_WAITING_EVENTFLAG		= 5,
		THREAD_STATUS_WAITING_MESSAGEBOX	= 6,
		THREAD_STATUS_WAIT_VBLANK_START		= 7,
		THREAD_STATUS_WAIT_VBLANK_END		= 8,
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

								CIopBios(CMIPS&, uint8*, uint32);
	virtual						~CIopBios();

	int32						LoadModule(const char*);
	int32						LoadModule(uint32);
	int32						LoadModuleFromHost(uint8*);
	int32						UnloadModule(uint32);
	int32						StartModule(uint32, const char*, const char*, uint32);
	int32						StopModule(uint32);
	bool						IsModuleHle(uint32) const;
	int32						SearchModuleByName(const char*) const;
	void						ProcessModuleReset(const std::string&);

	bool						TryGetImageVersionFromPath(const std::string&, unsigned int*);
	bool						TryGetImageVersionFromContents(const std::string&, unsigned int*);

	void						HandleException();
	void						HandleInterrupt();

	void						Reschedule();

	void						CountTicks(uint32);
	uint64						GetCurrentTime();
	uint64						MilliSecToClock(uint32);
	uint64						MicroSecToClock(uint32);
	uint64						ClockToMicroSec(uint64);

	void						NotifyVBlankStart();
	void						NotifyVBlankEnd();

	void						Reset(const Iop::SifManPtr&);

	virtual void				SaveState(Framework::CZipArchiveWriter&);
	virtual void				LoadState(Framework::CZipArchiveReader&);

	bool						IsIdle();

	Iop::CIoman*				GetIoman();
	Iop::CCdvdman*				GetCdvdman();
	Iop::CLoadcore*				GetLoadcore();
#ifdef _IOP_EMULATE_MODULES
	Iop::CPadMan*				GetPadman();
	Iop::CCdvdfsv*				GetCdvdfsv();
#endif
	bool						RegisterModule(const Iop::ModulePtr&);

	uint32						CreateThread(uint32, uint32, uint32, uint32);
	void						DeleteThread(uint32);
	int32						StartThread(uint32, uint32);
	int32						StartThreadArgs(uint32, uint32, uint32);
	void						ExitThread();
	uint32						TerminateThread(uint32);
	void						DelayThread(uint32);
	void						DelayThreadTicks(uint32);
	uint32						SetAlarm(uint32, uint32, uint32);
	uint32						CancelAlarm(uint32, uint32);
	THREAD*						GetThread(uint32);
	int32						GetCurrentThreadId();
	int32						GetCurrentThreadIdRaw() const;
	void						ChangeThreadPriority(uint32, uint32);
	uint32						ReferThreadStatus(uint32, uint32, bool);
	void						SleepThread();
	uint32						WakeupThread(uint32, bool);

	void						SleepThreadTillVBlankStart();
	void						SleepThreadTillVBlankEnd();

	uint32						CreateSemaphore(uint32, uint32);
	uint32						DeleteSemaphore(uint32);
	uint32						SignalSemaphore(uint32, bool);
	uint32						WaitSemaphore(uint32);
	uint32						PollSemaphore(uint32);
	uint32						ReferSemaphoreStatus(uint32, uint32);

	uint32						CreateEventFlag(uint32, uint32, uint32);
	uint32						SetEventFlag(uint32, uint32, bool);
	uint32						ClearEventFlag(uint32, uint32);
	uint32						WaitEventFlag(uint32, uint32, uint32, uint32);
	uint32						ReferEventFlagStatus(uint32, uint32);
	bool						ProcessEventFlag(uint32, uint32&, uint32, uint32*);

	uint32						CreateMessageBox();
	uint32						DeleteMessageBox(uint32);
	uint32						SendMessageBox(uint32, uint32);
	uint32						ReceiveMessageBox(uint32, uint32);
	uint32						PollMessageBox(uint32, uint32);
	uint32						ReferMessageBoxStatus(uint32, uint32);

	uint32						CreateVpl(uint32);
	uint32						DeleteVpl(uint32);
	uint32						pAllocateVpl(uint32, uint32);
	uint32						FreeVpl(uint32, uint32);
	uint32						ReferVplStatus(uint32, uint32);
	uint32						GetVplFreeSize(uint32);

	int32						RegisterIntrHandler(uint32, uint32, uint32, uint32);
	int32						ReleaseIntrHandler(uint32);

	void						TriggerCallback(uint32 address, uint32 arg0, uint32 arg1);

#ifdef DEBUGGER_INCLUDED
	void						LoadDebugTags(Framework::Xml::CNode*);
	void						SaveDebugTags(Framework::Xml::CNode*);

	BiosDebugModuleInfoArray	GetModulesDebugInfo() const override;
	BiosDebugThreadInfoArray	GetThreadsDebugInfo() const override;
#endif

	typedef boost::signals2::signal<void (uint32)> ModuleStartedEvent;

	ModuleStartedEvent			OnModuleStarted;

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
		RESIDENT_END			= 0,
		NO_RESIDENT_END			= 1,
		REMOVABLE_RESIDENT_END	= 2,
	};

	enum
	{
		MAX_THREAD				= 128,
		MAX_MEMORYBLOCK			= 256,
		MAX_SEMAPHORE			= 128,
		MAX_EVENTFLAG			= 64,
		MAX_INTRHANDLER			= 32,
		MAX_MESSAGEBOX			= 32,
		MAX_VPL					= 16,
		MAX_MODULESTARTREQUEST	= 32,
		MAX_LOADEDMODULE		= 32,
	};

	enum WEF_FLAGS
	{
		WEF_AND		= 0x00,
		WEF_OR		= 0x01,
		WEF_CLEAR	= 0x10,
	};

	struct SEMAPHORE
	{
		uint32			isValid;
		uint32			id;
		uint32			count;
		uint32			maxCount;
		uint32			waitCount;
	};

	struct SEMAPHORE_STATUS
	{
		uint32			attrib;
		uint32			option;
		uint32			initCount;
		uint32			maxCount;
		uint32			currentCount;
		uint32			numWaitThreads;
	};

	struct EVENTFLAG
	{
		uint32			isValid;
		uint32			id;
		uint32			attributes;
		uint32			options;
		uint32			value;
	};

	struct EVENTFLAGINFO
	{
		uint32			attributes;
		uint32			options;
		uint32			initBits;
		uint32			currBits;
		uint32			numThreads;
	};

	struct INTRHANDLER
	{
		uint32			isValid;
		uint32			line;
		uint32			mode;
		uint32			handler;
		uint32			arg;
	};

	struct MESSAGEBOX
	{
		uint32			isValid;
		uint32			nextMsgPtr;
		uint32			numMessage;
	};

	struct MESSAGEBOX_STATUS
	{
		uint32			attr;
		uint32			option;
		uint32			numWaitThread;
		uint32			numMessage;
		uint32			messagePtr;
		uint32			unused[2];
	};

	struct MESSAGE_HEADER
	{
		uint32			nextMsgPtr;
		uint8			priority;
		uint8			unused[3];
	};

	struct VPL
	{
		uint32			isValid;
		uint32			attr;
		uint32			option;
		uint32			poolPtr;
		uint32			size;
		uint32			headBlockId;
	};

	enum VPL_ATTR
	{
		VPL_ATTR_THFIFO       = 0x000,
		VPL_ATTR_THPRI        = 0x001,
		VPL_ATTR_THMODE_MASK  = 0x001,
		VPL_ATTR_MEMBTM       = 0x200,
		VPL_ATTR_MEMMODE_MASK = 0x200,
		VPL_ATTR_VALID_MASK   = (VPL_ATTR_THMODE_MASK | VPL_ATTR_MEMMODE_MASK)
	};

	struct VPL_PARAM
	{
		uint32			attr;
		uint32			option;
		uint32			size;
	};

	struct VPL_STATUS
	{
		uint32			attr;
		uint32			option;
		uint32			size;
		uint32			freeSize;
		uint32			numWaitThreads;
		uint32			unused[3];
	};

	struct MODULESTARTREQUEST
	{
		enum 
		{
			MAX_PATH_SIZE = 256,
			MAX_ARGS_SIZE = 256
		};

		uint32			nextPtr;
		uint32			moduleId;
		uint32			stopRequest;
		char			path[MAX_PATH_SIZE];
		uint32			argsLength;
		char			args[MAX_ARGS_SIZE];
	};

	struct LOADEDMODULE
	{
		enum
		{
			MAX_NAME_SIZE = 0x100,
		};

		uint32					isValid;
		char					name[MAX_NAME_SIZE];
		uint32					start;
		uint32					entryPoint;
		uint32					gp;
		MODULE_STATE			state;
		MODULE_RESIDENT_STATE	residentState;
	};

	struct IOPMOD
	{
		uint32			module;
		uint32			startAddress;
		uint32			gp;
		uint32			textSectionSize;
		uint32			dataSectionSize;
		uint32			bssSectionSize;
		uint16			moduleStructAttr;
		char			moduleName[256];
	};

	enum
	{
		IOPMOD_SECTION_ID = 0x70000080,
	};

	enum KERNEL_RESULT_CODES
	{
		KERNEL_RESULT_OK                     =    0,
		KERNEL_RESULT_ERROR                  =   -1,
		KERNEL_RESULT_ERROR_ILLEGAL_CONTEXT  = -100,
		KERNEL_RESULT_ERROR_ILLEGAL_INTRCODE = -101,
		KERNEL_RESULT_ERROR_FOUND_HANDLER    = -104,
		KERNEL_RESULT_ERROR_NOTFOUND_HANDLER = -105,
		KERNEL_RESULT_ERROR_NO_MEMORY        = -400,
		KERNEL_RESULT_ERROR_ILLEGAL_ATTR     = -401,
		KERNEL_RESULT_ERROR_UNKNOWN_THID     = -407,
		KERNEL_RESULT_ERROR_UNKNOWN_MBXID    = -410,
		KERNEL_RESULT_ERROR_UNKNOWN_VPLID    = -411,
		KERNEL_RESULT_ERROR_SEMA_ZERO        = -419,
		KERNEL_RESULT_ERROR_ILLEGAL_MEMSIZE  = -427,
	};

	typedef COsStructManager<THREAD> ThreadList;
	typedef COsStructManager<Iop::MEMORYBLOCK> MemoryBlockList;
	typedef COsStructManager<SEMAPHORE> SemaphoreList;
	typedef COsStructManager<EVENTFLAG> EventFlagList;
	typedef COsStructManager<INTRHANDLER> IntrHandlerList;
	typedef COsStructManager<MESSAGEBOX> MessageBoxList;
	typedef COsStructManager<VPL> VplList;
	typedef COsStructManager<LOADEDMODULE> LoadedModuleList;
	typedef std::map<std::string, Iop::ModulePtr> IopModuleMapType;
	typedef std::pair<uint32, uint32> ExecutableRange;

	void							LoadThreadContext(uint32);
	void							SaveThreadContext(uint32);
	uint32							GetNextReadyThread();
	void							ReturnFromException();

	uint32							FindIntrHandler(uint32);

	void							LinkThread(uint32);
	void							UnlinkThread(uint32);

	uint32&							ThreadLinkHead() const;
	uint64&							CurrentTime() const;
	uint32&							ModuleStartRequestHead() const;
	uint32&							ModuleStartRequestFree() const;

	int32							LoadModule(CELF&, const char*);
	uint32							LoadExecutable(CELF&, ExecutableRange&);
	unsigned int					GetElfProgramToLoad(CELF&);
	void							RelocateElf(CELF&, uint32);
	std::string						ReadModuleName(uint32);
	void							DeleteModules();

	int32							LoadHleModule(const Iop::ModulePtr&);

	uint32							AssembleThreadFinish(CMIPSAssembler&);
	uint32							AssembleReturnFromException(CMIPSAssembler&);
	uint32							AssembleIdleFunction(CMIPSAssembler&);
	uint32							AssembleModuleStarterThreadProc(CMIPSAssembler&);
	uint32							AssembleAlarmThreadProc(CMIPSAssembler&);

	void							InitializeModuleStarter();
	void							ProcessModuleStart();
	void							FinishModuleStart();
	void							RequestModuleStart(bool, uint32, const char*, const char*, unsigned int);

#ifdef DEBUGGER_INCLUDED
	void							PrepareModuleDebugInfo(CELF&, const ExecutableRange&, const std::string&, const std::string&);
	BiosDebugModuleInfoIterator		FindModuleDebugInfo(const std::string&);
	BiosDebugModuleInfoIterator		FindModuleDebugInfo(uint32, uint32);
#endif

	CMIPS&							m_cpu;
	uint8*							m_ram;
	uint32							m_ramSize;
	uint32							m_threadFinishAddress;
	uint32							m_returnFromExceptionAddress;
	uint32							m_idleFunctionAddress;
	uint32							m_moduleStarterThreadProcAddress;
	uint32							m_alarmThreadProcAddress;

	uint32							m_moduleStarterThreadId;

	bool							m_rescheduleNeeded = false;
	LoadedModuleList				m_loadedModules;
	ThreadList						m_threads;
	MemoryBlockList					m_memoryBlocks;
	SemaphoreList					m_semaphores;
	EventFlagList					m_eventFlags;
	IntrHandlerList					m_intrHandlers;
	MessageBoxList					m_messageBoxes;
	VplList							m_vpls;

	IopModuleMapType				m_modules;

	OsVariableWrapper<uint32>		m_currentThreadId;

#ifdef DEBUGGER_INCLUDED
	BiosDebugModuleInfoArray		m_moduleTags;
#endif
	Iop::SifManPtr					m_sifMan;
	Iop::SifCmdPtr					m_sifCmd;
	Iop::StdioPtr					m_stdio;
	Iop::IomanPtr					m_ioman;
	Iop::CdvdmanPtr					m_cdvdman;
	Iop::SysmemPtr					m_sysmem;
	Iop::ModloadPtr					m_modload;
	Iop::LoadcorePtr				m_loadcore;
	Iop::ModulePtr					m_libsd;
#ifdef _IOP_EMULATE_MODULES
	Iop::FileIoPtr					m_fileIo;
	Iop::PadManPtr					m_padman;
	Iop::MtapManPtr					m_mtapman;
	Iop::CdvdfsvPtr					m_cdvdfsv;
#endif
};

typedef std::shared_ptr<CIopBios> IopBiosPtr;
