#include "Iop_Timrman.h"

using namespace Iop;
using namespace std;

CTimrman::CTimrman()
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
    case 20:
        context.m_State.nGPR[CMIPS::V0].nD0 = 0;
        printf("Timrman: Install handler.\r\n");
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
       
    }
}
