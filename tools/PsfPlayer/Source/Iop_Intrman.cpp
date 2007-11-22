#include "Iop_Intrman.h"
#include "Log.h"

#define LOGNAME "iop_intrman"

using namespace Iop;
using namespace std;

CIntrman::CIntrman()
{

}

CIntrman::~CIntrman()
{

}

string CIntrman::GetId() const
{
    return "intrman";
}

void CIntrman::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    default:
        CLog::GetInstance().Print(LOGNAME, "%0.8X: Unknown function (%d) called.\r\n", context.m_State.nPC, functionId);
        break;
    }
}
