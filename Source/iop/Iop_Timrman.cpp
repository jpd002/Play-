#include <assert.h>
#include "Iop_Timrman.h"
#include "Iop_Intc.h"
#include "../Log.h"
#include "IopBios.h"
#include "Iop_RootCounters.h"

#define LOG_NAME ("iop_timrman")

#define FUNCTION_ALLOCHARDTIMER			"AllocHardTimer"
#define FUNCTION_SETTIMERCALLBACK		"SetTimerCallback"
#define FUNCTION_UNKNOWNTIMERFUNCTION22	"UnknownTimerFunction22"
#define FUNCTION_UNKNOWNTIMERFUNCTION23	"UnknownTimerFunction23"

using namespace Iop;

CTimrman::CTimrman(CIopBios& bios) :
m_bios(bios)
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

#define TIMER_ID (2)

int CTimrman::AllocHardTimer(int source, int size, int prescale)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOG_NAME, FUNCTION_ALLOCHARDTIMER "(source = %d, size = %d, prescale = %d).\r\n",
        source, size, prescale);
#endif
    return TIMER_ID;
}

int CTimrman::SetTimerCallback(CMIPS& context, int timerId, uint32 unknown, uint32 handler, uint32 arg)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTIMERCALLBACK "(timerId = %d, unknown = %d, handler = 0x%0.8X, arg = 0x%0.8X).\r\n",
		timerId, unknown, handler, arg);
#endif
	assert(timerId == TIMER_ID);
	m_bios.RegisterIntrHandler(CIntc::LINE_RTC2, 0, handler, arg);
	
	//Enable timer
	context.m_pMemoryMap->SetWord(CRootCounters::CNT2_BASE + CRootCounters::CNT_COUNT,	0x0000);
	context.m_pMemoryMap->SetWord(CRootCounters::CNT2_BASE + CRootCounters::CNT_MODE,	0x0258);
	context.m_pMemoryMap->SetWord(CRootCounters::CNT2_BASE + CRootCounters::CNT_TARGET,	0x4400);

	uint32 mask = context.m_pMemoryMap->GetWord(CIntc::MASK0);
	mask |= (1 << CIntc::LINE_RTC2);
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
