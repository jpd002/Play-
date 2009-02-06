#include "Iop_SifCmd.h"
#include "IopBios.h"
#include "../Log.h"
#include "../StructCollectionStateFile.h"
#include <boost/lexical_cast.hpp>

using namespace Iop;
using namespace std;

#define LOG_NAME                            ("iop_sifcmd")
#define STATE_MODULES                       ("iop_sifcmd/modules.xml")
#define STATE_MODULE                        ("Module")
#define STATE_MODULE_SERVER_DATA_ADDRESS    ("ServerDataAddress")

#define CUSTOM_RETURNFROMRPCINVOKE  0x666

#define MODULE_NAME                     "sifcmd"
#define MODULE_VERSION                  0x101

#define FUNCTION_SIFINITRPC             "SifInitRpc"
#define FUNCTION_SIFREGISTERRPC         "SifRegisterRpc"
#define FUNCTION_SIFSETRPCQUEUE         "SifSetRpcQueue"
#define FUNCTION_SIFRPCLOOP             "SifRpcLoop"
#define FUNCTION_RETURNFROMRPCINVOKE    "ReturnFromRpcInvoke"

#define INVOKE_PARAMS_SIZE          0x4000
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

    BuildExportTable();
}

CSifCmd::~CSifCmd()
{
    ClearServers();
}

void CSifCmd::LoadState(CZipArchiveReader& archive)
{
    ClearServers();

    CStructCollectionStateFile modulesFile(*archive.BeginReadFile(STATE_MODULES));
    {
        for(CStructCollectionStateFile::StructIterator structIterator(modulesFile.GetStructBegin());
            structIterator != modulesFile.GetStructEnd(); structIterator++)
        {
            const CStructFile& structFile(structIterator->second);
            uint32 serverDataAddress = structFile.GetRegister32(STATE_MODULE_SERVER_DATA_ADDRESS);
            SIFRPCSERVERDATA* serverData(reinterpret_cast<SIFRPCSERVERDATA*>(m_ram + serverDataAddress));
            CSifDynamic* module = new CSifDynamic(*this, serverDataAddress);
            m_servers.push_back(module);
            m_sifMan.RegisterModule(serverData->serverId, module);
        }
    }
}

void CSifCmd::SaveState(CZipArchiveWriter& archive)
{
    CStructCollectionStateFile* modulesFile = new CStructCollectionStateFile(STATE_MODULES);
    {
        int moduleIndex = 0;
        for(DynamicModuleList::iterator serverIterator(m_servers.begin());
            serverIterator != m_servers.end(); serverIterator++)
        {
            CSifDynamic* module(*serverIterator);
            string moduleName = string(STATE_MODULE) + boost::lexical_cast<string>(moduleIndex++);
            CStructFile moduleStruct;
            {
                uint32 serverDataAddress = module->GetServerDataAddress();
                moduleStruct.SetRegister32(STATE_MODULE_SERVER_DATA_ADDRESS, serverDataAddress); 
            }
            modulesFile->InsertStruct(moduleName.c_str(), moduleStruct);
        }
    }
    archive.InsertFile(modulesFile);
}

string CSifCmd::GetId() const
{
    return MODULE_NAME;
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
    case CUSTOM_RETURNFROMRPCINVOKE:
        return FUNCTION_RETURNFROMRPCINVOKE;
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
    case CUSTOM_RETURNFROMRPCINVOKE:
        ReturnFromRpcInvoke(context);
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function called (%d).\r\n", 
            functionId);
        break;
    }
}

void CSifCmd::ClearServers()
{
    for(DynamicModuleList::iterator serverIterator(m_servers.begin());
        m_servers.end() != serverIterator; serverIterator++)
    {
        CSifDynamic* server(*serverIterator);
        SIFRPCSERVERDATA* serverData(reinterpret_cast<SIFRPCSERVERDATA*>(m_ram + server->GetServerDataAddress()));
        m_sifMan.UnregisterModule(serverData->serverId);
        delete (*serverIterator);
    }
    m_servers.clear();
}

void CSifCmd::BuildExportTable()
{
    uint32* exportTable = reinterpret_cast<uint32*>(m_ram + m_trampolineAddr);
    *(exportTable++) = 0x41E00000;
    *(exportTable++) = 0;
    *(exportTable++) = MODULE_VERSION;
    strcpy(reinterpret_cast<char*>(exportTable), MODULE_NAME);
    exportTable += (strlen(MODULE_NAME) + 3) / 4;

    {
        m_returnFromRpcInvokeAddr = reinterpret_cast<uint8*>(exportTable) - m_ram;
        CMIPSAssembler assembler(exportTable);
        assembler.JR(CMIPS::RA);
        assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_RETURNFROMRPCINVOKE);
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
    thread.context.gpr[CMIPS::S0] = serverDataAddr;
    thread.context.gpr[CMIPS::RA] = m_returnFromRpcInvokeAddr;
    m_bios.WakeupThread(dataQueue->threadId, true);
}

void CSifCmd::ReturnFromRpcInvoke(CMIPS& context)
{
    SIFRPCSERVERDATA* serverData = reinterpret_cast<SIFRPCSERVERDATA*>(&m_ram[context.m_State.nGPR[CMIPS::S0].nV0]);
    uint8* returnData = m_ram + context.m_State.nGPR[CMIPS::V0].nV0;
    m_bios.SleepThread();
    m_sifMan.SendCallReply(serverData->serverId, returnData);
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
