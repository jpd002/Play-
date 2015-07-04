#pragma once

#include <memory>
#include <list>
#include "../MIPSAssembler.h"
#include "../MIPS.h"
#include "../ELF.h"
#include "../OsStructManager.h"
#include "Iop_BiosBase.h"
#include "Iop_SifMan.h"
#include "Iop_SifCmd.h"
#include "Iop_Ioman.h"
#include "Iop_Cdvdman.h"
#include "Iop_Stdio.h"
#include "Iop_Sysmem.h"
#include "Iop_Modload.h"
#include "Iop_Loadcore.h"
#include "Iop_LibSd.h"
#include "Iop_Dynamic.h"
#ifdef _IOP_EMULATE_MODULES
#include "Iop_FileIo.h"
#include "Iop_PadMan.h"
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

	enum THREAD_STATUS_OFFSETS
	{
		THREAD_INFO_ATTRIBUTE				= 0,
		THREAD_INFO_OPTION					= 1,
		THREAD_INFO_STATUS					= 2,
		THREAD_INFO_THREAD					= 3,
		THREAD_INFO_STACK					= 4,
		THREAD_INFO_STACKSIZE				= 5,
		THREAD_INFO_INITPRIORITY			= 7,
		THREAD_INFO_PRIORITY				= 8
	};

								CIopBios(CMIPS&, uint8*, uint32);
	virtual						~CIopBios();

	int32						LoadModule(const char*);
	int32						LoadModule(uint32);
	int32						LoadModuleFromHost(uint8*);
	int32						UnloadModule(uint32);
	int32						StartModule(uint32, const char*, const char*, unsigned int);
	int32						StopModule(uint32);
	int32						SearchModuleByName(const char*) const;
	void						ProcessModuleReset(const std::string&);

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

	void						Reset(Iop::CSifMan*);

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
	void						RegisterDynamicModule(Iop::CDynamic*);

	uint32						CreateThread(uint32, uint32, uint32, uint32);
	void						DeleteThread(uint32);
	int32						StartThread(uint32, uint32);
	void						ExitThread();
	uint32						TerminateThread(uint32);
	void						DelayThread(uint32);
	void						DelayThreadTicks(uint32);
	uint32						SetAlarm(uint32, uint32, uint32);
	uint32						CancelAlarm(uint32, uint32);
	THREAD*						GetThread(uint32);
	uint32						GetCurrentThreadId() const;
	void						ChangeThreadPriority(uint32, uint32);
	uint32						ReferThreadStatus(uint32, uint32);
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
	uint32						SendMessageBox(uint32, uint32);
	uint32						ReceiveMessageBox(uint32, uint32);
	uint32						PollMessageBox(uint32, uint32);

	bool						RegisterIntrHandler(uint32, uint32, uint32, uint32);
	bool						ReleaseIntrHandler(uint32);

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

	enum class MODULE_STATE : uint32
	{
		STOPPED,
		STARTED
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
		MAX_SEMAPHORE			= 64,
		MAX_EVENTFLAG			= 64,
		MAX_INTRHANDLER			= 32,
		MAX_MESSAGEBOX			= 32,
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

	typedef COsStructManager<THREAD> ThreadList;
	typedef COsStructManager<SEMAPHORE> SemaphoreList;
	typedef COsStructManager<EVENTFLAG> EventFlagList;
	typedef COsStructManager<INTRHANDLER> IntrHandlerList;
	typedef COsStructManager<MESSAGEBOX> MessageBoxList;
	typedef COsStructManager<LOADEDMODULE> LoadedModuleList;
	typedef std::map<std::string, Iop::CModule*> IopModuleMapType;
	typedef std::list<Iop::CDynamic*> DynamicIopModuleListType;
	typedef std::pair<uint32, uint32> ExecutableRange;
	typedef std::shared_ptr<Iop::CModule> ModulePtr;

	void							RegisterModule(Iop::CModule*);
	void							ClearDynamicModules();

	void							ExitCurrentThread();
	void							LoadThreadContext(uint32);
	void							SaveThreadContext(uint32);
	uint32							GetNextReadyThread();
	void							ReturnFromException();

	uint32							FindIntrHandler(uint32);

	void							LinkThread(uint32);
	void							UnlinkThread(uint32);

	uint32&							ThreadLinkHead() const;
	uint32&							CurrentThreadId() const;
	uint64&							CurrentTime() const;
	uint32&							ModuleStartRequestHead() const;
	uint32&							ModuleStartRequestFree() const;

	int32							LoadModule(CELF&, const char*);
	uint32							LoadExecutable(CELF&, ExecutableRange&);
	unsigned int					GetElfProgramToLoad(CELF&);
	void							RelocateElf(CELF&, uint32);
	std::string						ReadModuleName(uint32);
	void							DeleteModules();

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
	SemaphoreList					m_semaphores;
	EventFlagList					m_eventFlags;
	IntrHandlerList					m_intrHandlers;
	MessageBoxList					m_messageBoxes;

	IopModuleMapType				m_modules;
	DynamicIopModuleListType		m_dynamicModules;

#ifdef DEBUGGER_INCLUDED
	BiosDebugModuleInfoArray		m_moduleTags;
#endif
	Iop::CSifMan*					m_sifMan;
	Iop::CSifCmd*					m_sifCmd;
	Iop::CStdio*					m_stdio;
	Iop::CIoman*					m_ioman;
	Iop::CCdvdman*					m_cdvdman;
	Iop::CSysmem*					m_sysmem;
	Iop::CModload*					m_modload;
	Iop::CLoadcore*					m_loadcore;
	ModulePtr						m_libsd;
#ifdef _IOP_EMULATE_MODULES
	Iop::CFileIo*					m_fileIo;
	Iop::CPadMan*					m_padman;
	Iop::CCdvdfsv*					m_cdvdfsv;
#endif
};

typedef std::shared_ptr<CIopBios> IopBiosPtr;
