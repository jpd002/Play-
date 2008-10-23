#include "Iop_Thevent.h"

using namespace Iop;
using namespace std;

CThevent::CThevent(CIopBios& bios, uint8* ram) :
m_bios(bios),
m_ram(ram)
{

}

CThevent::~CThevent()
{

}

string CThevent::GetId() const
{
    return "thevent";
}

void CThevent::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(
            CreateEventFlag(reinterpret_cast<EVENT*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]))
            );
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

uint32 CThevent::CreateEventFlag(EVENT* eventPtr)
{
    return 3;
}
