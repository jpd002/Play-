#pragma once

#include <string>
#include <boost/signals2.hpp>
#include "ELF.h"
#include "MIPS.h"
#include "GSHandler.h"
#include "SIF.h"
#include "BiosDebugInfoProvider.h"

class CIopBios;

class CPS2OS : public CBiosDebugInfoProvider
{
public:
	typedef std::vector<std::string> ArgumentList;

	typedef boost::signals2::signal<void (const char*, const ArgumentList&)> RequestLoadExecutableEvent;

												CPS2OS(CMIPS&, uint8*, uint8*, CGSHandler*&, CSIF&, CIopBios&);
	virtual										~CPS2OS();

	void										Initialize();
	void										Release();

	bool										IsIdle() const;

	void										DumpIntcHandlers();
	void										DumpDmacHandlers();

	void										BootFromFile(const char*);
	void										BootFromCDROM(const ArgumentList&);
	CELF*										GetELF();
	const char*									GetExecutableName() const;
	std::pair<uint32, uint32>					GetExecutableRange() const;
	uint32										LoadExecutable(const char*, const char*);

	void										HandleInterrupt();
	void										HandleSyscall();
	void										HandleReturnFromException();

	static uint32								TranslateAddress(CMIPS*, uint32);

#ifdef DEBUGGER_INCLUDED
	BiosDebugModuleInfoArray					GetModulesDebugInfo() const override;
	BiosDebugThreadInfoArray					GetThreadsDebugInfo() const override;
#endif

	boost::signals2::signal<void ()>			OnExecutableChange;
	boost::signals2::signal<void ()>			OnExecutableUnloading;
	boost::signals2::signal<void ()>			OnRequestInstructionCacheFlush;
	RequestLoadExecutableEvent					OnRequestLoadExecutable;

private:
	class CRoundRibbon
	{
	public:
												CRoundRibbon(void*, uint32);
												~CRoundRibbon();
		unsigned int							Insert(uint32, uint32);
		void									Remove(unsigned int);
		unsigned int							Begin();

		class ITERATOR
		{
		public:
												ITERATOR(CRoundRibbon*);
			ITERATOR&							operator =(unsigned int);
			ITERATOR&							operator ++(int);
			uint32								GetWeight();
			uint32								GetValue();
			unsigned int						GetIndex();
			bool								IsEnd();

		private:
			CRoundRibbon*						m_ribbon;
			unsigned int						m_index;
		};

	private:
		struct NODE
		{
			uint32								value;
			uint32								weight;
			unsigned int						indexNext;
			unsigned int						valid;
		};

		NODE*									GetNode(unsigned int);
		unsigned int							GetNodeIndex(NODE*);
		NODE*									AllocateNode();
		void									FreeNode(NODE*);

		NODE*									m_node;
		uint32									m_maxNode;
	};

	struct SEMAPHOREPARAM
	{
		uint32									count;
		uint32									maxCount;
		uint32									initCount;
		uint32									waitThreads;
		uint32									attributes;
		uint32									options;
	};

	struct THREADPARAM
	{
		uint32									status;
		uint32									threadProc;
		uint32									stackBase;
		uint32									stackSize;
		uint32									gp;
		uint32									priority;
		uint32									currentPriority;
	};

	struct SEMAPHORE
	{
		uint32									valid;
		uint32									count;
		uint32									maxCount;
		uint32									waitCount;
	};

	struct THREAD
	{
		uint32									valid;
		uint32									status;
		uint32									contextPtr;
		uint32									stackBase;
		uint32									heapBase;
		uint32									epc;
		uint32									priority;
		uint32									semaWait;
		uint32									wakeUpCount;
		uint32									scheduleID;
		uint32									stackSize;
		uint32									quota;
	};

	enum STACKRES
	{
		STACKRES = 0x2A0,
	};

	//Castlevania: CoD relies on the fact that the GPRs are stored
	//in order starting from R0 all the way up to RA because it read/writes
	//values directly in the thread context
	struct THREADCONTEXT
	{
		enum
		{
			COP1_REG_COUNT = 0x1C
		};

		uint128									gpr[0x20];
		uint128									hi;
		uint128									lo;
		uint32									sa;
		uint32									fcsr;
		uint32									cop1a;
		uint32									reserved3;
		uint32									cop1[COP1_REG_COUNT];
	};
	static_assert(sizeof(THREADCONTEXT) == STACKRES, "Size of THREADCONTEXT must be STACKRES");

	struct DMACHANDLER
	{
		uint32									valid;
		uint32									channel;
		uint32									address;
		uint32									arg;
		uint32									gp;
	};

	struct INTCHANDLER
	{
		uint32									valid;
		uint32									cause;
		uint32									address;
		uint32									arg;
		uint32									gp;
	};

	struct DECI2HANDLER
	{
		uint32									valid;
		uint32									device;
		uint32									bufferAddr;
	};

#ifdef DEBUGGER_INCLUDED
	struct SYSCALL_NAME
	{
		uint32									id;
		const char*								name;
	};
#endif

	enum SYSCALL_REGS
	{
		SC_RETURN = 2,
		SC_PARAM0 = 4,
		SC_PARAM1 = 5,
		SC_PARAM2 = 6,
		SC_PARAM3 = 7,
		SC_PARAM4 = 8
	};

	enum MAX
	{
		MAX_THREAD			= 256,
		MAX_SEMAPHORE		= 256,
		MAX_DMACHANDLER		= 128,
		MAX_INTCHANDLER		= 128,
		MAX_DECI2HANDLER	= 32,
	};

	enum THREAD_STATUS
	{
		THREAD_RUNNING				= 0x01,
		THREAD_SLEEPING				= 0x02,
		THREAD_WAITING				= 0x03,
		THREAD_SUSPENDED			= 0x04,
		THREAD_SUSPENDED_WAITING	= 0x05,
		THREAD_SUSPENDED_SLEEPING	= 0x06,
		THREAD_ZOMBIE				= 0x07,
	};

	typedef void (CPS2OS::*SystemCallHandler)();

	void									LoadELF(Framework::CStream&, const char*, const ArgumentList&);

	void									LoadExecutableInternal();
	void									UnloadExecutable();

	void									ApplyPatches();

	void									DisassembleSysCall(uint8);
	std::string								GetSysCallDescription(uint8);

	static SystemCallHandler				m_sysCall[0x80];

	void									AssembleCustomSyscallHandler();
	void									AssembleInterruptHandler();
	void									AssembleDmacHandler();
	void									AssembleIntcHandler();
	void									AssembleThreadEpilog();
	void									AssembleWaitThreadProc();

	uint32*									GetCustomSyscallTable();

	void									CreateWaitThread();
	uint32									GetCurrentThreadId() const;
	void									SetCurrentThreadId(uint32);
	uint32									GetNextAvailableThreadId();
	THREAD*									GetThread(uint32) const;
	void									ThreadShakeAndBake();
	bool									ThreadHasAllQuotasExpired();
	void									ThreadSwitchContext(unsigned int);

	uint32									GetNextAvailableSemaphoreId();
	SEMAPHORE*								GetSemaphore(uint32);

	uint32									GetNextAvailableDmacHandlerId();
	DMACHANDLER*							GetDmacHandler(uint32);

	uint32									GetNextAvailableIntcHandlerId();
	INTCHANDLER*							GetIntcHandler(uint32);

	uint32									GetNextAvailableDeci2HandlerId();
	DECI2HANDLER*							GetDeci2Handler(uint32);

	//Various system calls
	void									sc_GsSetCrt();
	void									sc_LoadExecPS2();
	void									sc_AddIntcHandler();
	void									sc_RemoveIntcHandler();
	void									sc_AddDmacHandler();
	void									sc_RemoveDmacHandler();
	void									sc_EnableIntc();
	void									sc_DisableIntc();
	void									sc_EnableDmac();
	void									sc_DisableDmac();
	void									sc_CreateThread();
	void									sc_DeleteThread();
	void									sc_StartThread();
	void									sc_ExitThread();
	void									sc_TerminateThread();
	void									sc_ChangeThreadPriority();
	void									sc_RotateThreadReadyQueue();
	void									sc_GetThreadId();
	void									sc_ReferThreadStatus();
	void									sc_SleepThread();
	void									sc_WakeupThread();
	void									sc_SuspendThread();
	void									sc_ResumeThread();
	void									sc_SetupThread();
	void									sc_SetupHeap();
	void									sc_EndOfHeap();
	void									sc_CreateSema();
	void									sc_DeleteSema();
	void									sc_SignalSema();
	void									sc_WaitSema();
	void									sc_PollSema();
	void									sc_ReferSemaStatus();
	void									sc_FlushCache();
	void									sc_GsGetIMR();
	void									sc_GsPutIMR();
	void									sc_SetVSyncFlag();
	void									sc_SetSyscall();
	void									sc_SifDmaStat();
	void									sc_SifSetDma();
	void									sc_SifSetDChain();
	void									sc_SifSetReg();
	void									sc_SifGetReg();
	void									sc_Deci2Call();
	void									sc_GetMemorySize();
	void									sc_Unhandled();

	CELF*									m_elf;
	CMIPS&									m_ee;
	CRoundRibbon*							m_threadSchedule;

	std::string								m_executableName;
	ArgumentList							m_currentArguments;

	uint8*									m_ram;
	uint8*									m_bios;
	CGSHandler*&							m_gs;
	CSIF&									m_sif;
	CIopBios&								m_iopBios;

	uint32									m_semaWaitId;
	uint32									m_semaWaitCount;
	uint32									m_semaWaitCaller;
	uint32									m_semaWaitThreadId;

#ifdef DEBUGGER_INCLUDED
	static const SYSCALL_NAME				g_syscallNames[];
#endif
};
