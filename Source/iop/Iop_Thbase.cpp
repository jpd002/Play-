#include "Iop_Thbase.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME ("iop_thbase")

#define FUNCTION_CREATETHREAD				"CreateThread"
#define FUNCTION_STARTTHREAD				"StartThread"
#define FUNCTION_DELETETHREAD				"DeleteThread"
#define FUNCTION_EXITTHREAD					"ExitThread"
#define FUNCTION_TERMINATETHREAD			"TerminateThread"
#define FUNCTION_CHANGETHREADPRIORITY		"ChangeThreadPriority"
#define FUNCTION_GETTHREADID				"GetThreadId"
#define FUNCTION_REFERTHREADSTATUS			"ReferThreadStatus"
#define FUNCTION_SLEEPTHREAD				"SleepThread"
#define FUNCTION_WAKEUPTHREAD				"WakeupThread"
#define FUNCTION_IWAKEUPTHREAD				"iWakeupThread"
#define FUNCTION_DELAYTHREAD				"DelayThread"
#define FUNCTION_GETSYSTEMTIME				"GetSystemTime"
#define FUNCTION_SETALARM					"SetAlarm"
#define FUNCTION_CANCELALARM				"CancelAlarm"
#define FUNCTION_USECTOSYSCLOCK				"USecToSysClock"
#define FUNCTION_SYSCLOCKTOUSEC				"SysClockToUSec"
#define FUNCTION_GETCURRENTTHREADPRIORITY	"GetCurrentThreadPriority"

CThbase::CThbase(CIopBios& bios, uint8* ram) :
m_ram(ram),
m_bios(bios)
{

}

CThbase::~CThbase()
{

}

std::string CThbase::GetId() const
{
	return "thbase";
}

std::string CThbase::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_CREATETHREAD;
		break;
	case 5:
		return FUNCTION_DELETETHREAD;
		break;
	case 6:
		return FUNCTION_STARTTHREAD;
		break;
	case 8:
		return FUNCTION_EXITTHREAD;
		break;
	case 10:
		return FUNCTION_TERMINATETHREAD;
		break;
	case 14:
		return FUNCTION_CHANGETHREADPRIORITY;
		break;
	case 20:
		return FUNCTION_GETTHREADID;
		break;
	case 22:
		return FUNCTION_REFERTHREADSTATUS;
		break;
	case 24:
		return FUNCTION_SLEEPTHREAD;
		break;
	case 25:
		return FUNCTION_WAKEUPTHREAD;
		break;
	case 26:
		return FUNCTION_IWAKEUPTHREAD;
		break;
	case 33:
		return FUNCTION_DELAYTHREAD;
		break;
	case 34:
		return FUNCTION_GETSYSTEMTIME;
		break;
	case 35:
		return FUNCTION_SETALARM;
		break;
	case 37:
		return FUNCTION_CANCELALARM;
		break;
	case 39:
		return FUNCTION_USECTOSYSCLOCK;
		break;
	case 40:
		return FUNCTION_SYSCLOCKTOUSEC;
		break;
	case 42:
		return FUNCTION_GETCURRENTTHREADPRIORITY;
		break;
	default:
		return "unknown";
		break;
	}
}

void CThbase::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CreateThread(
			reinterpret_cast<THREAD*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
			));
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DeleteThread(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(StartThread(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ExitThread());
		break;
	case 10:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(TerminateThread(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	case 14:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ChangeThreadPriority(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	case 20:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(GetThreadId());
		break;
	case 22:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ReferThreadStatus(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	case 24:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SleepThread());
		break;
	case 25:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(WakeupThread(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	case 26:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(iWakeupThread(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	case 33:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DelayThread(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	case 34:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(GetSystemTime(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	case 35:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SetAlarm(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0
			));
		break;
	case 37:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CancelAlarm(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	case 39:
		USecToSysClock(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 40:
		SysClockToUSec(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 42:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(GetCurrentThreadPriority());
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called at (%0.8X).\r\n", functionId, context.m_State.nPC);
		break;
	}
}

uint32 CThbase::CreateThread(const THREAD* thread)
{
	return m_bios.CreateThread(thread->threadProc, thread->priority, thread->stackSize, thread->options);
}

uint32 CThbase::DeleteThread(uint32 threadId)
{
	m_bios.DeleteThread(threadId);
	return 0;
}

int32 CThbase::StartThread(uint32 threadId, uint32 param)
{
	return m_bios.StartThread(threadId, param);
}

uint32 CThbase::ExitThread()
{
	m_bios.ExitThread();
	return 0;
}

uint32 CThbase::TerminateThread(uint32 threadId)
{
	return m_bios.TerminateThread(threadId);
}

uint32 CThbase::ChangeThreadPriority(uint32 threadId, uint32 newPrio)
{
	m_bios.ChangeThreadPriority(threadId, newPrio);
	return 0;
}

uint32 CThbase::DelayThread(uint32 delay)
{
	m_bios.DelayThread(delay);
	return 1;
}

uint32 CThbase::GetThreadId()
{
	return m_bios.GetCurrentThreadId();
}

uint32 CThbase::ReferThreadStatus(uint32 threadId, uint32 statusPtr)
{
	return m_bios.ReferThreadStatus(threadId, statusPtr);
}

uint32 CThbase::SleepThread()
{
	m_bios.SleepThread();
	return 1;
}

uint32 CThbase::WakeupThread(uint32 threadId)
{
	return m_bios.WakeupThread(threadId, false);
}

uint32 CThbase::iWakeupThread(uint32 threadId)
{
	return m_bios.WakeupThread(threadId, true);
}

uint32 CThbase::GetSystemTime(uint32 resultAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "%d : GetSystemTime(result);\r\n",
		m_bios.GetCurrentThreadId());
#endif
	uint64* result = NULL;
	if(resultAddr != 0)
	{
		result = reinterpret_cast<uint64*>(&m_ram[resultAddr]);
	}
	if(result != NULL)
	{
		(*result) = m_bios.GetCurrentTime();
	}
	return 1;
}

uint32 CThbase::SetAlarm(uint32 timePtr, uint32 alarmFunction, uint32 param)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "%d : SetAlarm(timePtr = 0x%0.8X, alarmFunction = 0x%0.8X, param = 0x%0.8X);\r\n",
		m_bios.GetCurrentThreadId(), timePtr, alarmFunction, param);
#endif
	return m_bios.SetAlarm(timePtr, alarmFunction, param);
}

uint32 CThbase::CancelAlarm(uint32 alarmFunction, uint32 param)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "%d : CancelAlarm(alarmFunction = 0x%0.8X, param = 0x%0.8X);\r\n",
		m_bios.GetCurrentThreadId(), alarmFunction, param);
#endif
	return m_bios.CancelAlarm(alarmFunction, param);
}

void CThbase::USecToSysClock(uint32 usec, uint32 timePtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "%d : USecToSysClock(usec = 0x%0.8X, timePtr = 0x%0.8X);\r\n",
		m_bios.GetCurrentThreadId(), usec, timePtr);
#endif
	uint64* time = (timePtr != 0) ? reinterpret_cast<uint64*>(&m_ram[timePtr]) : NULL;
	if(time != NULL)
	{
		(*time) = m_bios.MicroSecToClock(usec);
	}
}

void CThbase::SysClockToUSec(uint32 timePtr, uint32 secPtr, uint32 usecPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "%d : SysClockToUSec(time = 0x%0.8X, sec = 0x%0.8X, usec = 0x%0.8X);\r\n",
		m_bios.GetCurrentThreadId(), timePtr, secPtr, usecPtr);
#endif
	uint64* time = (timePtr != 0) ? reinterpret_cast<uint64*>(&m_ram[timePtr]) : NULL;
	uint32* sec = (secPtr != 0) ? reinterpret_cast<uint32*>(&m_ram[secPtr]) : NULL;
	uint32* usec = (usecPtr != 0) ? reinterpret_cast<uint32*>(&m_ram[usecPtr]) : NULL;
	if(time != NULL)
	{
		uint64 totalusec = m_bios.ClockToMicroSec(*time);
		if(sec != NULL)
		{
			*sec = static_cast<uint32>(totalusec / 1000000);
		}
		if(usec != NULL)
		{
			*usec = static_cast<uint32>(totalusec % 1000000);
		}
	}
}

uint32 CThbase::GetCurrentThreadPriority()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "%d : " FUNCTION_GETCURRENTTHREADPRIORITY "();\r\n",
		m_bios.GetCurrentThreadId());
#endif
	CIopBios::THREAD* currentThread = m_bios.GetThread(m_bios.GetCurrentThreadId());
	if(currentThread == NULL) return -1;
	return currentThread->priority;
}
