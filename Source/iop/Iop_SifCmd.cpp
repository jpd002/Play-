#include "Iop_SifCmd.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;
using namespace std;

#define LOG_NAME                    ("iop_sifcmd")

#define FUNCTION_SIFINITRPC         "SifInitRpc"
#define FUNCTION_SIFREGISTERRPC     "SifRegisterRpc"
#define FUNCTION_SIFSETRPCQUEUE     "SifSetRpcQueue"
#define FUNCTION_SIFRPCLOOP         "SifRpcLoop"

CSifCmd::CSifCmd(CIopBios& bios) :
m_bios(bios)
{

}

CSifCmd::~CSifCmd()
{

}

string CSifCmd::GetId() const
{
    return "sifcmd";
}

string CSifCmd::GetFunctionName(unsigned int functionId) const
{
    switch(functionId)
    {
    case 14:
        return FUNCTION_SIFINITRPC;
        break;
    case 17:
        return FUNCTION_SIFREGISTERRPC;
        break;
    case 19:
        return FUNCTION_SIFSETRPCQUEUE;
        break;
    case 22:
        return FUNCTION_SIFRPCLOOP;
        break;
    default:
        return "unknown";
        break;
    }
}

void CSifCmd::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 17:
        SifRegisterRpc(context);
        break;
    case 22:
        SifRpcLoop(context.m_State.nGPR[CMIPS::A0].nV0);
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function called (%d).\r\n", 
            functionId);
        break;
    }
}

void CSifCmd::SifRegisterRpc(CMIPS& context)
{
    uint32 serverData   = context.m_State.nGPR[CMIPS::A0].nV0;
    uint32 serverId     = context.m_State.nGPR[CMIPS::A1].nV0;
    uint32 function     = context.m_State.nGPR[CMIPS::A2].nV0;
    uint32 buffer       = context.m_State.nGPR[CMIPS::A3].nV0;
    uint32 cfunction    = context.m_State.nGPR[CMIPS::T0].nV0;
    uint32 cbuffer      = context.m_State.nGPR[CMIPS::T1].nV0;
    uint32 queue        = context.m_State.nGPR[CMIPS::T2].nV0;

    CLog::GetInstance().Print(LOG_NAME, "SifRegisterRpc(serverData = 0x%0.8X, serverId = 0x%0.8X, function = 0x%0.8X, buffer = 0x%0.8X, cfunction = 0x%0.8X, cbuffer = 0x%0.8X, queue = 0x%0.8X);\r\n",
        serverData, serverId, function, buffer, cfunction, cbuffer, queue);

    context.m_State.nGPR[CMIPS::V0].nD0 = 0;
}

void CSifCmd::SifRpcLoop(uint32 queueAddr)
{
    CLog::GetInstance().Print(LOG_NAME, "SifRpcLoop(queue = 0x%0.8X);\r\n",
        queueAddr);
    m_bios.SleepThread();
}
