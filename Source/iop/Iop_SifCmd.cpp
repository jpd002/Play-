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

#define INVOKE_PARAMS_SIZE          0x800
#define TRAMPOLINE_SIZE             0x800

CSifCmd::CSifCmd(CIopBios& bios, CSifMan& sifMan, CSysmem& sysMem, uint8* ram) :
m_sifMan(sifMan),
m_bios(bios),
m_sysMem(sysMem),
m_ram(ram)
{
    m_memoryBufferAddr = m_sysMem.AllocateMemory(INVOKE_PARAMS_SIZE + TRAMPOLINE_SIZE, 0);
    m_invokeParamsAddr = m_memoryBufferAddr;
    m_trampolineAddr = m_invokeParamsAddr + INVOKE_PARAMS_SIZE;
}

CSifCmd::~CSifCmd()
{
    for(DynamicModuleList::iterator serverIterator(m_servers.begin());
        m_servers.end() != serverIterator; serverIterator++)
    {
        delete (*serverIterator);
    }
    m_servers.clear();
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
    case 19:
        SifSetRpcQueue(context.m_State.nGPR[CMIPS::A0].nV0,
            context.m_State.nGPR[CMIPS::A1].nV0);
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

void CSifCmd::ProcessInvocation(uint32 serverDataAddr, uint32 methodId, uint32* params, uint32 size)
{
    SIFRPCSERVERDATA* serverData = reinterpret_cast<SIFRPCSERVERDATA*>(&m_ram[serverDataAddr]);
    SIFRPCDATAQUEUE* dataQueue = reinterpret_cast<SIFRPCDATAQUEUE*>(&m_ram[serverData->queueAddr]);

    //Copy params
    assert(size <= INVOKE_PARAMS_SIZE);
    uint32 copySize = min<uint32>(size, INVOKE_PARAMS_SIZE);
    memcpy(&m_ram[m_invokeParamsAddr], params, copySize);
    CIopBios::THREAD& thread(m_bios.GetThread(dataQueue->threadId));
    thread.context.epc = serverData->function;
    thread.context.gpr[CMIPS::A0] = methodId;
    thread.context.gpr[CMIPS::A1] = m_invokeParamsAddr;
    thread.context.gpr[CMIPS::A2] = size;
    m_bios.WakeupThread(dataQueue->threadId, true);
//    thread.context.gpr[CMIPS::RA] = 
}

void CSifCmd::SifRegisterRpc(CMIPS& context)
{
    uint32 serverDataAddr   = context.m_State.nGPR[CMIPS::A0].nV0;
    uint32 serverId         = context.m_State.nGPR[CMIPS::A1].nV0;
    uint32 function         = context.m_State.nGPR[CMIPS::A2].nV0;
    uint32 buffer           = context.m_State.nGPR[CMIPS::A3].nV0;
    uint32 cfunction        = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10);
    uint32 cbuffer          = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14);
    uint32 queueAddr        = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x18);

    CLog::GetInstance().Print(LOG_NAME, "SifRegisterRpc(serverData = 0x%0.8X, serverId = 0x%0.8X, function = 0x%0.8X, buffer = 0x%0.8X, cfunction = 0x%0.8X, cbuffer = 0x%0.8X, queue = 0x%0.8X);\r\n",
        serverDataAddr, serverId, function, buffer, cfunction, cbuffer, queueAddr);

    if(!(serverId & 0x80000000))
    {
        CSifDynamic* module = new CSifDynamic(*this, serverDataAddr);
        m_servers.push_back(module);
        m_sifMan.RegisterModule(serverId, module);
    }

    if(serverDataAddr != 0)
    {
        SIFRPCSERVERDATA* serverData = reinterpret_cast<SIFRPCSERVERDATA*>(&m_ram[serverDataAddr]);
        serverData->serverId    = serverId;
        serverData->function    = function;
        serverData->buffer      = buffer;
        serverData->cfunction   = cfunction;
        serverData->cbuffer     = cbuffer;
        serverData->queueAddr   = queueAddr;
    }

    if(queueAddr != 0)
    {
        SIFRPCDATAQUEUE* dataQueue = reinterpret_cast<SIFRPCDATAQUEUE*>(&m_ram[queueAddr]);
        assert(dataQueue->serverDataStart == 0);
        dataQueue->serverDataStart = serverDataAddr;
    }

    context.m_State.nGPR[CMIPS::V0].nD0 = 0;
}

void CSifCmd::SifSetRpcQueue(uint32 queueAddr, uint32 threadId)
{
    CLog::GetInstance().Print(LOG_NAME, "SifSetRpcQueue(queue = 0x%0.8X, threadId = %d);\r\n",
        queueAddr, threadId);

    if(queueAddr != 0)
    {
        SIFRPCDATAQUEUE* dataQueue = reinterpret_cast<SIFRPCDATAQUEUE*>(&m_ram[queueAddr]);
        dataQueue->threadId = threadId;
    }
}

void CSifCmd::SifRpcLoop(uint32 queueAddr)
{
    CLog::GetInstance().Print(LOG_NAME, "SifRpcLoop(queue = 0x%0.8X);\r\n",
        queueAddr);
    m_bios.SleepThread();
}
