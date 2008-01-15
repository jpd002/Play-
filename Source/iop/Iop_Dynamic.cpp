#include "Iop_Dynamic.h"

using namespace std;
using namespace Iop;

CDynamic::CDynamic(uint32* exportTable) :
m_exportTable(exportTable),
m_name(reinterpret_cast<char*>(m_exportTable) + 12)
{

}

CDynamic::~CDynamic()
{
    
}

string CDynamic::GetId() const
{
    return m_name;
}

void CDynamic::Invoke(CMIPS& context, unsigned int functionId)
{
    uint32 functionAddress = m_exportTable[5 + functionId];
    context.m_State.nGPR[CMIPS::RA].nD0 = context.m_State.nPC;
    context.m_State.nPC = functionAddress;
}
