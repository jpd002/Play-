#ifndef _PS2OS_H_
#define _PS2OS_H_

#include <string>
#include <boost/signals2.hpp>
#include "ELF.h"
#include "MIPS.h"
#include "GSHandler.h"
#include "SIF.h"
#include "MipsModule.h"
#include "DebugThreadInfo.h"

class CIopBios;

class CPS2OS
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
	MipsModuleList								GetModuleList() const;
	DebugThreadInfoArray						GetThreadInfos() const;

	void										ExceptionHandler();
	void										SysCallHandler();
	static uint32								TranslateAddress(CMIPS*, uint32);

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
			CRoundRibbon*						m_pRibbon;
			unsigned int						m_nIndex;
		};

	private:
		struct NODE
		{
			uint32								nValue;
			uint32								nWeight;
			unsigned int						nIndexNext;
			unsigned int						nValid;
		};

		NODE*									GetNode(unsigned int);
		unsigned int							GetNodeIndex(NODE*);
		NODE*									AllocateNode();
		void									FreeNode(NODE*);

		NODE*									m_pNode;
		uint32									m_nMaxNode;
	};

	struct SEMAPHOREPARAM
	{
		uint32									nCount;
		uint32									nMaxCount;
		uint32									nInitCount;
		uint32									nWaitThreads;
		uint32									nAttributes;
		uint32									nOptions;
	};

	struct THREADPARAM
	{
		uint32									nStatus;
		uint32									nThreadProc;
		uint32									nStackBase;
		uint32									nStackSize;
		uint32									nGP;
		uint32									nPriority;
		uint32									nCurrentPriority;
	};

	struct SEMAPHORE
	{
		uint32									nValid;
		uint32									nCount;
		uint32									nMaxCount;
		uint32									nWaitCount;
	};

	struct THREAD
	{
		uint32									nValid;
		uint32									nStatus;
		uint32									nContextPtr;
		uint32									nStackBase;
		uint32									nHeapBase;
		uint32									nEPC;
		uint32									nPriority;
		uint32									nSemaWait;
		uint32									nWakeUpCount;
		uint32									nScheduleID;
		uint32									nStackSize;
		uint32									nQuota;
	};

	struct THREADCONTEXT
	{
		uint128									nGPR[0x20];
		uint128									nHI;
		uint128									nLO;
		uint32									nSA;
		uint32									nFCSR;
		uint32									nCOP1A;
		uint32									nReserved3;
		uint32									nCOP1[0x1C];
	};

	struct DMACHANDLER
	{
		uint32									nValid;
		uint32									nChannel;
		uint32									nAddress;
		uint32									nArg;
		uint32									nGP;
	};

	struct INTCHANDLER
	{
		uint32									nValid;
		uint32									nCause;
		uint32									nAddress;
		uint32									nArg;
		uint32									nGP;
	};

	struct DECI2HANDLER
	{
		uint32									nValid;
		uint32									nDevice;
		uint32									nBufferAddr;
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

	enum STACKRES
	{
		STACKRES = 0x2A0,
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
		THREAD_RUNNING		= 0x01,
		THREAD_SUSPENDED	= 0x02,
		THREAD_WAITING		= 0x03,
		THREAD_ZOMBIE		= 0x04,
	};

	typedef void (CPS2OS::*SystemCallHandler)();

	void									LoadELF(Framework::CStream&, const char*, const ArgumentList&);

	void									LoadExecutable();
	void									UnloadExecutable();

	void									ApplyPatches();

	void									DisassembleSysCall(uint8);
	std::string								GetSysCallDescription(uint8);

	static SystemCallHandler				m_pSysCall[0x80];

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

	CELF*									m_pELF;
	CMIPS&									m_ee;
	CRoundRibbon*							m_pThreadSchedule;

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

#endif
