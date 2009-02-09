#include "Iop_Thsema.h"
#include "IopBios.h"

using namespace Iop;
using namespace std;

#define FUNCTION_CREATESEMAPHORE    "CreateSemaphore"
#define FUNCTION_DELETESEMAPHORE    "DeleteSemaphore"
#define FUNCTION_SIGNALSEMAPHORE    "SignalSemaphore"
#define FUNCTION_ISIGNALSEMAPHORE   "iSignalSemaphore"
#define FUNCTION_WAITSEMAPHORE      "WaitSemaphore"

CThsema::CThsema(CIopBios& bios, uint8* ram) :
m_bios(bios),
m_ram(ram)
{

}

CThsema::~CThsema()
{

}

string CThsema::GetId() const
{
    return "thsemap";
}

string CThsema::GetFunctionName(unsigned int functionId) const
{
    switch(functionId)
    {
    case 4:
        return FUNCTION_CREATESEMAPHORE;
        break;
    case 5:
        return FUNCTION_DELETESEMAPHORE;
        break;
    case 6:
        return FUNCTION_SIGNALSEMAPHORE;
        break;
    case 7:
        return FUNCTION_ISIGNALSEMAPHORE;
        break;
    case 8:
        return FUNCTION_WAITSEMAPHORE; 
        break;
    default:
    	return "unknown";
        break;
    }
}

void CThsema::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CreateSemaphore(
            reinterpret_cast<SEMAPHORE*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
            ));
        break;
    case 5:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DeleteSemaphore(
            context.m_State.nGPR[CMIPS::A0].nV0
            ));
        break;
    case 6:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SignalSemaphore(
            context.m_State.nGPR[CMIPS::A0].nV0
            ));
        break;
	case 7:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(iSignalSemaphore(
            context.m_State.nGPR[CMIPS::A0].nV0
            ));
		break;
    case 8:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(WaitSemaphore(
            context.m_State.nGPR[CMIPS::A0].nV0
            ));
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

uint32 CThsema::CreateSemaphore(const SEMAPHORE* semaphore)
{
    return m_bios.CreateSemaphore(semaphore->initialCount, semaphore->maxCount);
}

uint32 CThsema::DeleteSemaphore(uint32 semaphoreId)
{
    return m_bios.DeleteSemaphore(semaphoreId);
}

uint32 CThsema::SignalSemaphore(uint32 semaphoreId)
{
    return m_bios.SignalSemaphore(semaphoreId, false);
}

uint32 CThsema::iSignalSemaphore(uint32 semaphoreId)
{
	return m_bios.SignalSemaphore(semaphoreId, true);
}

uint32 CThsema::WaitSemaphore(uint32 semaphoreId)
{
    return m_bios.WaitSemaphore(semaphoreId);
}
