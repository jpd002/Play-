#include "Iop_Ioman.h"

using namespace Iop;

CIoman::CIoman(uint8* ram) :
m_ram(ram)
{

}

CIoman::~CIoman()
{

}

std::string CIoman::GetId() const
{
    return "ioman";
}

void CIoman::RegisterDevice(const char* name, Ioman::CDevice* device)
{
    m_devices[name] = device;
}

uint32 CIoman::Open(uint32 flags, const char* path)
{
    return 0xFFFFFFFF;
}

void CIoman::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Open(
            context.m_State.nGPR[CMIPS::A1].nV[0],
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV[0]])
            ));
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}
