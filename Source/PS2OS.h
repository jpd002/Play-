#ifndef _PS2OS_H_
#define _PS2OS_H_

#include "ELF.h"
#include "Str.h"
#include "Event.h"
#include "List.h"
#include "MIPS.h"

class CPS2OS
{
public:
												CPS2OS();
												~CPS2OS();

	static void									Initialize();
	static void									Release();
	static bool									IsInitialized();

	static void									DumpThreadSchedule();
	static void									DumpIntcHandlers();
	static void									DumpDmacHandlers();

	static void									BootFromFile(const char*);
	static void									BootFromCDROM();
	static CELF*								GetELF();
	static const char*							GetExecutableName();

	static void									ExceptionHandler();
	static uint32								TranslateAddress(CMIPS*, uint32, uint32);

	static Framework::CEvent<int>				m_OnExecutableChange;
	static Framework::CEvent<int>				m_OnExecutableUnloading;

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

	static void									LoadELF(Framework::CStream*, const char*);

	static void									LoadExecutable();
	static void									UnloadExecutable();

	static void									SaveExecutableConfig();
	static void									LoadExecutableConfig();

	static void									ApplyPatches();

	static void									SysCallHandler();
	static void									DisassembleSysCall(uint8);

	static void									(*m_pSysCall[0x80])();

	static void									AssembleCustomSyscallHandler();
	static void									AssembleInterruptHandler();
	static void									AssembleDmacHandler();
	static void									AssembleIntcHandler();
	static void									AssembleThreadEpilog();

	static uint32*								GetCustomSyscallTable();

	static uint32								GetCurrentThreadId();
	static void									SetCurrentThreadId(uint32);
	static uint32								GetNextAvailableThreadId();
	static THREAD*								GetThread(uint32);
	static void									ElectThread(uint32);
	static uint32								GetNextReadyThread();

	static uint32								GetNextAvailableSemaphoreId();
	static SEMAPHORE*							GetSemaphore(uint32);

	static uint32								GetNextAvailableDmacHandlerId();
	static DMACHANDLER*							GetDmacHandler(uint32);

	static uint32								GetNextAvailableIntcHandlerId();
	static INTCHANDLER*							GetIntcHandler(uint32);

	static uint32								GetNextAvailableDeci2HandlerId();
	static DECI2HANDLER*						GetDeci2Handler(uint32);

	//Various system calls
	static void									sc_GsSetCrt();
	static void									sc_AddIntcHandler();
	static void									sc_RemoveIntcHandler();
	static void									sc_AddDmacHandler();
	static void									sc_RemoveDmacHandler();
	static void									sc_EnableIntc();
	static void									sc_DisableIntc();
	static void									sc_EnableDmac();
	static void									sc_DisableDmac();
	static void									sc_CreateThread();
	static void									sc_DeleteThread();
	static void									sc_StartThread();
	static void									sc_ExitThread();
	static void									sc_TerminateThread();
	static void									sc_ChangeThreadPriority();
	static void									sc_RotateThreadReadyQueue();
	static void									sc_GetThreadId();
	static void									sc_ReferThreadStatus();
	static void									sc_SleepThread();
	static void									sc_WakeupThread();
	static void									sc_RFU060();
	static void									sc_RFU061();
	static void									sc_EndOfHeap();
	static void									sc_CreateSema();
	static void									sc_DeleteSema();
	static void									sc_SignalSema();
	static void									sc_WaitSema();
	static void									sc_PollSema();
	static void									sc_ReferSemaStatus();
	static void									sc_FlushCache();
	static void									sc_GsPutIMR();
	static void									sc_SetVSyncFlag();
	static void									sc_SetSyscall();
	static void									sc_SifDmaStat();
	static void									sc_SifSetDma();
	static void									sc_SifSetDChain();
	static void									sc_SifSetReg();
	static void									sc_SifGetReg();
	static void									sc_Deci2Call();
	static void									sc_GetMemorySize();
	static void									sc_Unhandled();

	static bool									m_nInitialized;

	static CELF*								m_pELF;
	static CMIPS*								m_pCtx;
	static CRoundRibbon*						m_pThreadSchedule;

	static Framework::CStrA						m_sExecutableName;
};

#endif
