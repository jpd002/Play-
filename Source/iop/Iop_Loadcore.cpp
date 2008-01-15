#include "Iop_Loadcore.h"
#include "Iop_Dynamic.h"
#include "IopBios.h"

using namespace Iop;
using namespace std;

CLoadcore::CLoadcore(CIopBios& bios, uint8* ram) :
m_bios(bios),
m_ram(ram)
{

}

CLoadcore::~CLoadcore()
{

}

string CLoadcore::GetId() const
{
    return "loadcore";
}

void CLoadcore::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 6:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(RegisterLibraryEntries(
            reinterpret_cast<uint32*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
            ));
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

uint32 CLoadcore::RegisterLibraryEntries(uint32* exportTable)
{
    m_bios.RegisterModule(new CDynamic(exportTable));
    return 1;    
}
