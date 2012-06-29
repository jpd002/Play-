#include "Iop_SifCmd.h"
#include "IopBios.h"
#include "../SIF.h"
#include "../Log.h"
#include "../StructCollectionStateFile.h"
#include <boost/lexical_cast.hpp>

using namespace Iop;

#define LOG_NAME							("iop_sifcmd")
#define STATE_MODULES						("iop_sifcmd/modules.xml")
#define STATE_MODULE						("Module")
#define STATE_MODULE_SERVER_DATA_ADDRESS	("ServerDataAddress")

#define CUSTOM_RETURNFROMRPCINVOKE  0x666

#define MODULE_NAME						"sifcmd"
#define MODULE_VERSION					0x101

#define FUNCTION_SIFSENDCMD				"SifSendCmd"
#define FUNCTION_SIFINITRPC				"SifInitRpc"
#define FUNCTION_SIFBINDRPC				"SifBindRpc"
#define FUNCTION_SIFREGISTERRPC			"SifRegisterRpc"
#define FUNCTION_SIFCHECKSTATRPC		"SifCheckStatRpc"
#define FUNCTION_SIFSETRPCQUEUE			"SifSetRpcQueue"
#define FUNCTION_SIFRPCLOOP				"SifRpcLoop"
#define FUNCTION_SIFGETOTHERDATA		"SifGetOtherData"	
#define FUNCTION_RETURNFROMRPCINVOKE	"ReturnFromRpcInvoke"

#define INVOKE_PARAMS_SIZE			0x8000
#define TRAMPOLINE_SIZE				0x800

CSifCmd::CSifCmd(CIopBios& bios, CSifMan& sifMan, CSysmem& sysMem, uint8* ram) 
: m_sifMan(sifMan)
, m_bios(bios)
, m_sysMem(sysMem)
, m_ram(ram)
{
	m_memoryBufferAddr = m_sysMem.AllocateMemory(INVOKE_PARAMS_SIZE + TRAMPOLINE_SIZE, 0, 0);
	m_invokeParamsAddr = m_memoryBufferAddr;
	m_trampolineAddr = m_invokeParamsAddr + INVOKE_PARAMS_SIZE;

	BuildExportTable();
}

CSifCmd::~CSifCmd()
{
	ClearServers();
}

void CSifCmd::LoadState(Framework::CZipArchiveReader& archive)
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

void CSifCmd::SaveState(Framework::CZipArchiveWriter& archive)
{
	CStructCollectionStateFile* modulesFile = new CStructCollectionStateFile(STATE_MODULES);
	{
		int moduleIndex = 0;
		for(DynamicModuleList::iterator serverIterator(m_servers.begin());
			serverIterator != m_servers.end(); serverIterator++)
		{
			CSifDynamic* module(*serverIterator);
			std::string moduleName = std::string(STATE_MODULE) + boost::lexical_cast<std::string>(moduleIndex++);
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

std::string CSifCmd::GetId() const
{
	return MODULE_NAME;
}

std::string CSifCmd::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 12:
		return FUNCTION_SIFSENDCMD;
		break;
	case 14:
		return FUNCTION_SIFINITRPC;
		break;
	case 15:
		return FUNCTION_SIFBINDRPC;
		break;
	case 17:
		return FUNCTION_SIFREGISTERRPC;
		break;
	case 18:
		return FUNCTION_SIFCHECKSTATRPC;
		break;
	case 19:
		return FUNCTION_SIFSETRPCQUEUE;
		break;
	case 22:
		return FUNCTION_SIFRPCLOOP;
		break;
	case 23:
		return FUNCTION_SIFGETOTHERDATA;
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
	case 12:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifSendCmd(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0,
			context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10),
			context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14));
		break;
	case 15:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifBindRpc(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 17:
		SifRegisterRpc(context);
		break;
	case 18:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifCheckStatRpc(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 19:
		SifSetRpcQueue(context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 22:
		SifRpcLoop(context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 23:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifGetOtherData(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0,
			context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10));
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
	uint32 copySize = std::min<uint32>(size, INVOKE_PARAMS_SIZE);

	memcpy(&m_ram[serverData->buffer], params, copySize);
	CIopBios::THREAD* thread(m_bios.GetThread(dataQueue->threadId));

	assert(thread->status == CIopBios::THREAD_STATUS_SLEEPING);

	thread->context.epc = serverData->function;
	thread->context.gpr[CMIPS::A0] = methodId;
	thread->context.gpr[CMIPS::A1] = serverData->buffer;
	thread->context.gpr[CMIPS::A2] = size;
	thread->context.gpr[CMIPS::S0] = serverDataAddr;
	thread->context.gpr[CMIPS::RA] = m_returnFromRpcInvokeAddr;
	m_bios.WakeupThread(dataQueue->threadId, true);
	m_bios.Reschedule();
}

void CSifCmd::ReturnFromRpcInvoke(CMIPS& context)
{
	SIFRPCSERVERDATA* serverData = reinterpret_cast<SIFRPCSERVERDATA*>(&m_ram[context.m_State.nGPR[CMIPS::S0].nV0]);
	uint8* returnData = m_ram + context.m_State.nGPR[CMIPS::V0].nV0;
	m_bios.SleepThread();
	m_sifMan.SendCallReply(serverData->serverId, returnData);
}

uint32 CSifCmd::SifSendCmd(uint32 commandId, uint32 packetPtr, uint32 packetSize, uint32 srcExtraPtr, uint32 dstExtraPtr, uint32 sizeExtra)
{
	CLog::GetInstance().Print(LOG_NAME, "SifSendCmd(commandId = 0x%0.8X, packetPtr = 0x%0.8X, packetSize = 0x%0.8X, srcExtraPtr = 0x%0.8X, dstExtraPtr = 0x%0.8X, sizeExtra = 0x%0.8X);\r\n",
		commandId, packetPtr, packetSize, srcExtraPtr, dstExtraPtr, sizeExtra);

	assert(srcExtraPtr == 0);
	assert(dstExtraPtr == 0);
	assert(sizeExtra == 0);
	assert(packetSize >= 0x10);

	uint8* packetData = m_ram + packetPtr;
	CSIF::PACKETHDR* header = reinterpret_cast<CSIF::PACKETHDR*>(packetData);
	header->nCID = commandId;
	header->nSize = packetSize;
	header->nDest = 0;
	m_sifMan.SendPacket(packetData, packetSize);

	return 1;
}

uint32 CSifCmd::SifBindRpc(uint32 clientDataAddress, uint32 rpcNumber, uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, "SifBindRpc(clientData = 0x%0.8X, rpcNumber = 0x%0.8X, mode = 0x%0.8X);\r\n",
		clientDataAddress, rpcNumber, mode);
	//Set struct t_SifRpcServerData *server to 0
	*reinterpret_cast<uint32*>(&m_ram[clientDataAddress + 0x24]) = rpcNumber;
	return 0;
}

void CSifCmd::SifRegisterRpc(CMIPS& context)
{
	uint32 serverDataAddr	= context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 serverId			= context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 function			= context.m_State.nGPR[CMIPS::A2].nV0;
	uint32 buffer			= context.m_State.nGPR[CMIPS::A3].nV0;
	uint32 cfunction		= context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10);
	uint32 cbuffer			= context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14);
	uint32 queueAddr		= context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x18);

	CLog::GetInstance().Print(LOG_NAME, "SifRegisterRpc(serverData = 0x%0.8X, serverId = 0x%0.8X, function = 0x%0.8X, buffer = 0x%0.8X, cfunction = 0x%0.8X, cbuffer = 0x%0.8X, queue = 0x%0.8X);\r\n",
		serverDataAddr, serverId, function, buffer, cfunction, cbuffer, queueAddr);

	static const uint32 libSdServerId = 0x80000701;
	if(serverId == libSdServerId || ((serverId & 0x80000000) == 0))
	{
		CSifDynamic* module = new CSifDynamic(*this, serverDataAddr);
		m_servers.push_back(module);
		m_sifMan.RegisterModule(serverId, module);
	}

	if(serverDataAddr != 0)
	{
		SIFRPCSERVERDATA* serverData = reinterpret_cast<SIFRPCSERVERDATA*>(&m_ram[serverDataAddr]);
		serverData->serverId	= serverId;
		serverData->function	= function;
		serverData->buffer		= buffer;
		serverData->cfunction	= cfunction;
		serverData->cbuffer		= cbuffer;
		serverData->queueAddr	= queueAddr;
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

uint32 CSifCmd::SifCheckStatRpc(uint32 clientDataAddress)
{
	CLog::GetInstance().Print(LOG_NAME, "SifCheckStatRpc(clientData = 0x%0.8X);\r\n",
		clientDataAddress);
	return 0;
}

void CSifCmd::SifRpcLoop(uint32 queueAddr)
{
	CLog::GetInstance().Print(LOG_NAME, "SifRpcLoop(queue = 0x%0.8X);\r\n",
		queueAddr);
	m_bios.SleepThread();
}

uint32 CSifCmd::SifGetOtherData(uint32 packetPtr, uint32 src, uint32 dst, uint32 size, uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, "SifGetOtherData(packetPtr = 0x%0.8X, src = 0x%0.8X, dst = 0x%0.8X, size = 0x%0.8X, mode = %d);\r\n",
		packetPtr, src, dst, size, mode);
	m_sifMan.GetOtherData(dst, src, size);
	return 0;
}
