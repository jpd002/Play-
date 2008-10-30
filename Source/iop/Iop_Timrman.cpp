#include <assert.h>
#include "Iop_Timrman.h"
#include "Iop_Intc.h"
#include "Log.h"
#include "IopBios.h"
#include "RootCounters.h"

#define LOG_NAME ("iop_timrman")

using namespace Iop;
using namespace std;

CTimrman::CTimrman(CIopBios& bios) :
m_bios(bios)
{

}

CTimrman::~CTimrman()
{

}

string CTimrman::GetId() const
{
    return "timrman";
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
    CLog::GetInstance().Print(LOG_NAME, "AllocHardTimer(source = %d, size = %d, prescale = %d).\r\n",
        source, size, prescale);
#endif
    return TIMER_ID;
}

int CTimrman::SetTimerCallback(CMIPS& context, int timerId, uint32 unknown, uint32 handler, uint32 arg)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "SetTimerCallback(timerId = %d, unknown = %d, handler = 0x%0.8X, arg = 0x%0.8X.\r\n",
		timerId, unknown, handler, arg);
#endif
	assert(timerId == TIMER_ID);
	m_bios.RegisterIntrHandler(CIntc::LINE_RTC2, 0, handler, arg);
	
	//Enable timer
	context.m_pMemoryMap->SetWord(Psx::CRootCounters::CNT2_BASE + Psx::CRootCounters::CNT_COUNT,	0x0000);
	context.m_pMemoryMap->SetWord(Psx::CRootCounters::CNT2_BASE + Psx::CRootCounters::CNT_MODE,		0x0258);
	context.m_pMemoryMap->SetWord(Psx::CRootCounters::CNT2_BASE + Psx::CRootCounters::CNT_TARGET,	0x8000);

	uint32 mask = context.m_pMemoryMap->GetWord(CIntc::MASK0);
	mask |= (1 << CIntc::LINE_RTC2);
	context.m_pMemoryMap->SetWord(CIntc::MASK0, mask);
	return 0;
}
