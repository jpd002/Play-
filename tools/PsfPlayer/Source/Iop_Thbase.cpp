#include "Iop_Thbase.h"

using namespace Iop;
using namespace std;

CThbase::CThbase()
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
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}
