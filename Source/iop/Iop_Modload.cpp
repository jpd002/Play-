#include "Iop_Modload.h"
#include "IopBios.h"

using namespace Iop;
using namespace std;
using namespace Framework;

CModload::CModload(CIopBios& bios, uint8* ram) :
m_bios(bios),
m_ram(ram)
{

}

CModload::~CModload()
{

}

string CModload::GetId() const
{
    return "modload";
}

string CModload::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CModload::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 7:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(LoadStartModule(
            reinterpret_cast<const char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
            context.m_State.nGPR[CMIPS::A1].nV0,
            reinterpret_cast<const char*>(&m_ram[context.m_State.nGPR[CMIPS::A2].nV0]),
            reinterpret_cast<uint32*>(&m_ram[context.m_State.nGPR[CMIPS::A3].nV0])
            ));
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

uint32 CModload::LoadStartModule(const char* path, uint32 argsLength, const char* args, uint32* result)
{
    try
    {
        m_bios.LoadAndStartModule(path, args, argsLength);
    }
    catch(const exception& except)
    {
        printf("%s: Error occured while trying to load module '%s' : %s\r\n", __FUNCTION__, path, except.what());
    }
    return 0;
}
