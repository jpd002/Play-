#include "Iop_Timrman.h"
#include "Log.h"

#define LOG_NAME ("iop_timrman")

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
    case 4:
        context.m_State.nGPR[CMIPS::V0].nD0 = AllocHardTimer(
            context.m_State.nGPR[CMIPS::A0].nV0,
            context.m_State.nGPR[CMIPS::A1].nV0,
            context.m_State.nGPR[CMIPS::A2].nV0
            );
        break;
    case 20:
        context.m_State.nGPR[CMIPS::V0].nD0 = 0;
        CLog::GetInstance().Print(LOG_NAME, "Timrman: Install handler.\r\n");
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "(%0.8X): Unknown function (%d) called.\r\n", 
            context.m_State.nPC, functionId);
        break;
       
    }
}

int CTimrman::AllocHardTimer(int source, int size, int prescale)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOG_NAME, "AllocHardTimer(source = %d, size = %d, prescale = %d).\r\n",
        source, size, prescale);
#endif
    return 1;
}
