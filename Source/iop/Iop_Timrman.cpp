#include <assert.h>
#include "Iop_Timrman.h"
#include "Iop_Intc.h"
#include "../Log.h"
#include "IopBios.h"
#include "Iop_RootCounters.h"

#define LOG_NAME ("iop_timrman")

#define FUNCTION_ALLOCHARDTIMER			"AllocHardTimer"
#define FUNCTION_REFERHARDTIMER			"ReferHardTimer"
#define FUNCTION_SETTIMERMODE			"SetTimerMode"
#define FUNCTION_GETTIMERSTATUS			"GetTimerStatus"
#define FUNCTION_SETTIMERCOMPARE		"SetTimerCompare"
#define FUNCTION_GETHARDTIMERINTRCODE	"GetHardTimerIntrCode"
#define FUNCTION_SETTIMERCALLBACK		"SetTimerCallback"
#define FUNCTION_UNKNOWNTIMERFUNCTION22	"UnknownTimerFunction22"
#define FUNCTION_UNKNOWNTIMERFUNCTION23	"UnknownTimerFunction23"

using namespace Iop;

CTimrman::CTimrman(CIopBios& bios) 
: m_bios(bios)
{

}

CTimrman::~CTimrman()
{

}

std::string CTimrman::GetId() const
{
	return "timrman";
}

std::string CTimrman::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_ALLOCHARDTIMER;
		break;
	case 5:
		return FUNCTION_REFERHARDTIMER;
		break;
	case 7:
		return FUNCTION_SETTIMERMODE;
		break;
	case 8:
		return FUNCTION_GETTIMERSTATUS;
		break;
	case 11:
		return FUNCTION_SETTIMERCOMPARE;
		break;
	case 16:
		return FUNCTION_GETHARDTIMERINTRCODE;
		break;
	case 20:
		return FUNCTION_SETTIMERCALLBACK;
		break;
	case 22:
		return FUNCTION_UNKNOWNTIMERFUNCTION22;
		break;
	case 23:
		return FUNCTION_UNKNOWNTIMERFUNCTION23;
		break;
	default:
		return "unknown";
		break;
	}
}

void CTimrman::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = AllocHardTimer(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0
			);
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = ReferHardTimer(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0
			);
		break;
	case 7:
		SetTimerMode(
			context,
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			);
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = GetTimerStatus(
			context,
			context.m_State.nGPR[CMIPS::A0].nV0
			);
		break;
	case 11:
		SetTimerCompare(
			context,
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			);
		break;
	case 16:
		context.m_State.nGPR[CMIPS::V0].nD0 = GetHardTimerIntrCode(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 20:
		context.m_State.nGPR[CMIPS::V0].nD0 = SetTimerCallback(
			context,
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 22:
		context.m_State.nGPR[CMIPS::V0].nD0 = UnknownTimerFunction22(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0
			);
		break;
	case 23:
		context.m_State.nGPR[CMIPS::V0].nD0 = UnknownTimerFunction23(
			context.m_State.nGPR[CMIPS::A0].nV0
			);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "(%0.8X): Unknown function (%d) called.\r\n", 
			context.m_State.nPC, functionId);
		break;
	}
}

int CTimrman::AllocHardTimer(uint32 source, uint32 size, uint32 prescale)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ALLOCHARDTIMER "(source = %d, size = %d, prescale = %d).\r\n",
		source, size, prescale);
#endif
	for(unsigned int i = 0; i < CRootCounters::MAX_COUNTERS; i++)
	{
		if(
			(CRootCounters::g_counterSizes[i] == size) && 
			((CRootCounters::g_counterSources[i] & source) != 0)
			)
		{
			return (i + 1);
		}
	}
	return 0;
}

int CTimrman::ReferHardTimer(uint32 source, uint32 size, uint32 mode, uint32 modeMask)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REFERHARDTIMER "(source = %d, size = %d, mode = 0x%0.8X, mask = 0x%0.8X).\r\n",
		source, size, mode, modeMask);
#endif
	return 0;
}

void CTimrman::SetTimerMode(CMIPS& context, uint32 timerId, uint32 mode)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTIMERMODE "(timerId = %d, mode = 0x%0.8X).\r\n",
		timerId, mode);
#endif
	if(timerId == 0) return;
	timerId--;
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_MODE, mode);
}

int CTimrman::GetTimerStatus(CMIPS& context, uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETTIMERSTATUS "(timerId = %d).\r\n",
		timerId);
#endif
	if(timerId == 0) return 0;
	timerId--;
	return context.m_pMemoryMap->GetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_MODE) | 0x800;
}

void CTimrman::SetTimerCompare(CMIPS& context, uint32 timerId, uint32 compare)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTIMERCOMPARE "(timerId = %d, compare = 0x%0.8X).\r\n",
		timerId, compare);
#endif
	if(timerId == 0) return;
	timerId--;
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_COUNT, 0);
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_TARGET, compare);
}

int CTimrman::GetHardTimerIntrCode(uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETHARDTIMERINTRCODE "(timerId = %d).\r\n",
		timerId);
#endif
	if(timerId == 0) return CIntc::LINE_RTC0;
	timerId--;
	return CRootCounters::g_counterInterruptLines[timerId];
}

int CTimrman::SetTimerCallback(CMIPS& context, int timerId, uint32 target, uint32 handler, uint32 arg)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTIMERCALLBACK "(timerId = %d, target = %d, handler = 0x%0.8X, arg = 0x%0.8X).\r\n",
		timerId, target, handler, arg);
#endif
	if(timerId == 0) return 0;
	timerId--;

	uint32 timerInterruptLine = CRootCounters::g_counterInterruptLines[timerId];
	m_bios.RegisterIntrHandler(timerInterruptLine, 0, handler, arg);
	
	//Enable timer
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_COUNT,	0x0000);
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_MODE,		0x0058);
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_TARGET,	target);

	uint32 mask = context.m_pMemoryMap->GetWord(CIntc::MASK0);
	mask |= (1 << timerInterruptLine);
	context.m_pMemoryMap->SetWord(CIntc::MASK0, mask);
	return 0;
}

int CTimrman::UnknownTimerFunction22(uint32 timerId, uint32 unknown0, uint32 unknown1, uint32 unknown2)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_UNKNOWNTIMERFUNCTION22 "(timerId = %d, unknown0 = %d, unknown1 = %d, unknown2 = %d).\r\n",
		timerId, unknown0, unknown1, unknown2);
#endif
	return 0;
}

int CTimrman::UnknownTimerFunction23(uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_UNKNOWNTIMERFUNCTION23 "(timerId = %d).\r\n",
		timerId);
#endif
	return 0;
}
