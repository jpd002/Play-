#include "Iop_Thbase.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;
using namespace std;

#define LOG_NAME ("iop_thbase")

CThbase::CThbase(CIopBios& bios, uint8* ram) :
m_ram(ram),
m_bios(bios)
{

}

CThbase::~CThbase()
{

}

string CThbase::GetId() const
{
    return "thbase";
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
    case 6:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(StartThread(
            context.m_State.nGPR[CMIPS::A0].nV0,
            context.m_State.nGPR[CMIPS::A1].nV0
            ));
        break;
    case 20:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(GetThreadId());
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
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

uint32 CThbase::CreateThread(const THREAD* thread)
{
    return m_bios.CreateThread(thread->threadProc, thread->priority);
}

uint32 CThbase::StartThread(uint32 threadId, uint32 param)
{
    m_bios.StartThread(threadId, &param);
    return 1;
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
