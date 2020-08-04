#include <cstring>
#include "Iop_SifCmd.h"
#include "IopBios.h"
#include "../ee/SIF.h"
#include "../Log.h"
#include "../states/StructCollectionStateFile.h"

using namespace Iop;

#define LOG_NAME ("iop_sifcmd")
#define STATE_MODULES ("iop_sifcmd/modules.xml")
#define STATE_MODULE ("Module")
#define STATE_MODULE_SERVER_DATA_ADDRESS ("ServerDataAddress")

#define CUSTOM_FINISHEXECREQUEST 0x666
#define CUSTOM_FINISHEXECCMD 0x667
#define CUSTOM_FINISHBINDRPC 0x668
#define CUSTOM_SLEEPTHREAD 0x669
#define CUSTOM_DELAYTHREAD 0x66A

#define MODULE_NAME "sifcmd"
#define MODULE_VERSION 0x101

#define FUNCTION_SIFGETSREG "SifGetSreg"
#define FUNCTION_SIFSETCMDBUFFER "SifSetCmdBuffer"
#define FUNCTION_SIFADDCMDHANDLER "SifAddCmdHandler"
#define FUNCTION_SIFSENDCMD "SifSendCmd"
#define FUNCTION_ISIFSENDCMD "iSifSendCmd"
#define FUNCTION_SIFINITRPC "SifInitRpc"
#define FUNCTION_SIFBINDRPC "SifBindRpc"
#define FUNCTION_SIFCALLRPC "SifCallRpc"
#define FUNCTION_SIFREGISTERRPC "SifRegisterRpc"
#define FUNCTION_SIFCHECKSTATRPC "SifCheckStatRpc"
#define FUNCTION_SIFSETRPCQUEUE "SifSetRpcQueue"
#define FUNCTION_SIFGETNEXTREQUEST "SifGetNextRequest"
#define FUNCTION_SIFEXECREQUEST "SifExecRequest"
#define FUNCTION_SIFRPCLOOP "SifRpcLoop"
#define FUNCTION_SIFGETOTHERDATA "SifGetOtherData"
#define FUNCTION_SIFSENDCMDINTR "SifSendCmdIntr"
#define FUNCTION_FINISHEXECREQUEST "FinishExecRequest"
#define FUNCTION_FINISHEXECCMD "FinishExecCmd"
#define FUNCTION_FINISHBINDRPC "FinishBindRpc"
#define FUNCTION_SLEEPTHREAD "SleepThread"
#define FUNCTION_DELAYTHREAD "DelayThread"

#define SYSTEM_COMMAND_ID 0x80000000

CSifCmd::CSifCmd(CIopBios& bios, CSifMan& sifMan, CSysmem& sysMem, uint8* ram)
    : m_bios(bios)
    , m_sifMan(sifMan)
    , m_sysMem(sysMem)
    , m_ram(ram)
{
	m_moduleDataAddr = m_sysMem.AllocateMemory(sizeof(MODULEDATA), 0, 0);
	memset(m_ram + m_moduleDataAddr, 0, sizeof(MODULEDATA));
	m_trampolineAddr = m_moduleDataAddr + offsetof(MODULEDATA, trampoline);
	m_sendCmdExtraStructAddr = m_moduleDataAddr + offsetof(MODULEDATA, sendCmdExtraStruct);
	m_sysCmdBufferAddr = m_moduleDataAddr + offsetof(MODULEDATA, sysCmdBuffer);
	sifMan.SetModuleResetHandler([&](const std::string& path) { bios.ProcessModuleReset(path); });
	sifMan.SetCustomCommandHandler([&](uint32 commandHeaderAddr) { ProcessCustomCommand(commandHeaderAddr); });
	BuildExportTable();
}

CSifCmd::~CSifCmd()
{
	ClearServers();
}

void CSifCmd::LoadState(Framework::CZipArchiveReader& archive)
{
	ClearServers();

	auto modulesFile = CStructCollectionStateFile(*archive.BeginReadFile(STATE_MODULES));
	{
		for(CStructCollectionStateFile::StructIterator structIterator(modulesFile.GetStructBegin());
		    structIterator != modulesFile.GetStructEnd(); ++structIterator)
		{
			const auto& structFile(structIterator->second);
			uint32 serverDataAddress = structFile.GetRegister32(STATE_MODULE_SERVER_DATA_ADDRESS);
			auto serverData = reinterpret_cast<SIFRPCSERVERDATA*>(m_ram + serverDataAddress);
			auto module = new CSifDynamic(*this, serverDataAddress);
			m_servers.push_back(module);
			m_sifMan.RegisterModule(serverData->serverId, module);
		}
	}
}

void CSifCmd::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto modulesFile = new CStructCollectionStateFile(STATE_MODULES);
	{
		int moduleIndex = 0;
		for(const auto& module : m_servers)
		{
			auto moduleName = std::string(STATE_MODULE) + std::to_string(moduleIndex++);
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
	case 6:
		return FUNCTION_SIFGETSREG;
		break;
	case 8:
		return FUNCTION_SIFSETCMDBUFFER;
		break;
	case 10:
		return FUNCTION_SIFADDCMDHANDLER;
		break;
	case 12:
		return FUNCTION_SIFSENDCMD;
		break;
	case 13:
		return FUNCTION_ISIFSENDCMD;
		break;
	case 14:
		return FUNCTION_SIFINITRPC;
		break;
	case 15:
		return FUNCTION_SIFBINDRPC;
		break;
	case 16:
		return FUNCTION_SIFCALLRPC;
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
	case 20:
		return FUNCTION_SIFGETNEXTREQUEST;
		break;
	case 21:
		return FUNCTION_SIFEXECREQUEST;
		break;
	case 22:
		return FUNCTION_SIFRPCLOOP;
		break;
	case 23:
		return FUNCTION_SIFGETOTHERDATA;
		break;
	case 28:
		return FUNCTION_SIFSENDCMDINTR;
		break;
	case CUSTOM_FINISHEXECREQUEST:
		return FUNCTION_FINISHEXECREQUEST;
		break;
	case CUSTOM_FINISHEXECCMD:
		return FUNCTION_FINISHEXECCMD;
		break;
	case CUSTOM_FINISHBINDRPC:
		return FUNCTION_FINISHBINDRPC;
		break;
	case CUSTOM_SLEEPTHREAD:
		return FUNCTION_SLEEPTHREAD;
		break;
	case CUSTOM_DELAYTHREAD:
		return FUNCTION_DELAYTHREAD;
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
	case 6:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifGetSreg(
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifSetCmdBuffer(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 10:
		SifAddCmdHandler(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 12:
	case 13:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifSendCmd(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0,
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10),
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14));
		break;
	case 14:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFINITRPC "();\r\n");
		break;
	case 15:
		SifBindRpc(context);
		break;
	case 16:
		SifCallRpc(context);
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
	case 20:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SifGetNextRequest(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 21:
		SifExecRequest(context);
		break;
	case 22:
		SifRpcLoop(context);
		break;
	case 23:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifGetOtherData(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0,
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10));
		break;
	case 28:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifSendCmdIntr(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0,
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10),
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14),
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x18),
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x1C));
		break;
	case CUSTOM_FINISHEXECREQUEST:
		FinishExecRequest(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case CUSTOM_FINISHBINDRPC:
		FinishBindRpc(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case CUSTOM_FINISHEXECCMD:
		FinishExecCmd();
		break;
	case CUSTOM_SLEEPTHREAD:
		SleepThread();
		break;
	case CUSTOM_DELAYTHREAD:
		DelayThread(
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function called (%d).\r\n",
		                         functionId);
		break;
	}
}

void CSifCmd::ClearServers()
{
	for(const auto& server : m_servers)
	{
		auto serverData = reinterpret_cast<SIFRPCSERVERDATA*>(m_ram + server->GetServerDataAddress());
		m_sifMan.UnregisterModule(serverData->serverId);
		delete server;
	}
	m_servers.clear();
}

void CSifCmd::BuildExportTable()
{
	auto exportTable = reinterpret_cast<uint32*>(m_ram + m_trampolineAddr);
	*(exportTable++) = 0x41E00000;
	*(exportTable++) = 0;
	*(exportTable++) = MODULE_VERSION;
	strcpy(reinterpret_cast<char*>(exportTable), MODULE_NAME);
	exportTable += (strlen(MODULE_NAME) + 3) / 4;

	{
		CMIPSAssembler assembler(exportTable);

		//Trampoline for SifGetNextRequest
		uint32 sifGetNextRequestAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, 20);

		//Trampoline for SifExecRequest
		uint32 sifExecRequestAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, 21);

		uint32 finishExecRequestAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_FINISHEXECREQUEST);

		uint32 finishExecCmdAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_FINISHEXECCMD);

		uint32 finishBindRpcAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_FINISHBINDRPC);

		uint32 sleepThreadAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_SLEEPTHREAD);

		uint32 delayThreadAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_DELAYTHREAD);

		//Assemble SifRpcLoop
		{
			static const int16 stackAlloc = 0x10;

			m_sifRpcLoopAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);
			auto checkNextRequestLabel = assembler.CreateLabel();
			auto sleepThreadLabel = assembler.CreateLabel();

			assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
			assembler.SW(CMIPS::RA, 0x00, CMIPS::SP);
			assembler.SW(CMIPS::S0, 0x04, CMIPS::SP);
			assembler.ADDU(CMIPS::S0, CMIPS::A0, CMIPS::R0);

			//checkNextRequest:
			assembler.MarkLabel(checkNextRequestLabel);
			assembler.JAL(sifGetNextRequestAddr);
			assembler.ADDU(CMIPS::A0, CMIPS::S0, CMIPS::R0);
			assembler.BEQ(CMIPS::V0, CMIPS::R0, sleepThreadLabel);
			assembler.NOP();

			assembler.JAL(sifExecRequestAddr);
			assembler.ADDU(CMIPS::A0, CMIPS::V0, CMIPS::R0);

			//sleepThread:
			assembler.MarkLabel(sleepThreadLabel);
			assembler.JAL(sleepThreadAddr);
			assembler.NOP();
			assembler.BEQ(CMIPS::R0, CMIPS::R0, checkNextRequestLabel);
			assembler.NOP();

			assembler.LW(CMIPS::S0, 0x04, CMIPS::SP);
			assembler.LW(CMIPS::RA, 0x00, CMIPS::SP);
			assembler.JR(CMIPS::RA);
			assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);
		}

		//Assemble SifExecRequest
		{
			static const int16 stackAlloc = 0x20;

			m_sifExecRequestAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);

			assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
			assembler.SW(CMIPS::RA, 0x1C, CMIPS::SP);
			assembler.SW(CMIPS::S0, 0x18, CMIPS::SP);
			assembler.ADDU(CMIPS::S0, CMIPS::A0, CMIPS::R0);

			assembler.LW(CMIPS::A0, offsetof(SIFRPCSERVERDATA, rid), CMIPS::S0);
			assembler.LW(CMIPS::A1, offsetof(SIFRPCSERVERDATA, buffer), CMIPS::S0);
			assembler.LW(CMIPS::A2, offsetof(SIFRPCSERVERDATA, rsize), CMIPS::S0);
			assembler.LW(CMIPS::T0, offsetof(SIFRPCSERVERDATA, function), CMIPS::S0);
			assembler.JALR(CMIPS::T0);
			assembler.NOP();

			assembler.ADDU(CMIPS::A0, CMIPS::S0, CMIPS::R0);
			assembler.JAL(finishExecRequestAddr);
			assembler.ADDU(CMIPS::A1, CMIPS::V0, CMIPS::R0);

			assembler.LW(CMIPS::S0, 0x18, CMIPS::SP);
			assembler.LW(CMIPS::RA, 0x1C, CMIPS::SP);
			assembler.JR(CMIPS::RA);
			assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);
		}

		//Assemble SifExecCmdHandler
		{
			static const int16 stackAlloc = 0x20;

			m_sifExecCmdHandlerAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);

			assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
			assembler.SW(CMIPS::RA, 0x1C, CMIPS::SP);
			assembler.SW(CMIPS::S0, 0x18, CMIPS::SP);
			assembler.ADDU(CMIPS::S0, CMIPS::A0, CMIPS::R0);

			assembler.ADDU(CMIPS::A0, CMIPS::A1, CMIPS::R0); //A0 = Packet Address
			assembler.LW(CMIPS::A1, offsetof(SIFCMDDATA, data), CMIPS::S0);
			assembler.LW(CMIPS::T0, offsetof(SIFCMDDATA, sifCmdHandler), CMIPS::S0);
			assembler.JALR(CMIPS::T0);
			assembler.NOP();

			assembler.JAL(finishExecCmdAddr);
			assembler.NOP();

			assembler.LW(CMIPS::S0, 0x18, CMIPS::SP);
			assembler.LW(CMIPS::RA, 0x1C, CMIPS::SP);
			assembler.JR(CMIPS::RA);
			assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);
		}

		//Assemble SifBindRpc
		{
			static const int16 stackAlloc = 0x20;

			m_sifBindRpcAddr = (reinterpret_cast<uint8*>(exportTable) - m_ram) + (assembler.GetProgramSize() * 4);

			assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
			assembler.SW(CMIPS::RA, 0x1C, CMIPS::SP);
			assembler.SW(CMIPS::S0, 0x18, CMIPS::SP);
			assembler.SW(CMIPS::S1, 0x14, CMIPS::SP);
			assembler.ADDU(CMIPS::S0, CMIPS::A0, CMIPS::R0);
			assembler.ADDU(CMIPS::S1, CMIPS::A1, CMIPS::R0);

			assembler.LI(CMIPS::A0, 500);
			assembler.JAL(delayThreadAddr);
			assembler.NOP();

			assembler.ADDU(CMIPS::A0, CMIPS::S0, CMIPS::R0);
			assembler.ADDU(CMIPS::A1, CMIPS::S1, CMIPS::R0);
			assembler.JAL(finishBindRpcAddr);
			assembler.NOP();

			assembler.ADDU(CMIPS::V0, CMIPS::R0, CMIPS::R0);

			assembler.LW(CMIPS::S1, 0x14, CMIPS::SP);
			assembler.LW(CMIPS::S0, 0x18, CMIPS::SP);
			assembler.LW(CMIPS::RA, 0x1C, CMIPS::SP);
			assembler.JR(CMIPS::RA);
			assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);
		}
	}
}

void CSifCmd::ProcessInvocation(uint32 serverDataAddr, uint32 methodId, uint32* params, uint32 size)
{
	auto serverData = reinterpret_cast<SIFRPCSERVERDATA*>(m_ram + serverDataAddr);
	auto queueData = reinterpret_cast<SIFRPCQUEUEDATA*>(m_ram + serverData->queueAddr);

	//Copy params
	if(serverData->buffer != 0)
	{
		memcpy(&m_ram[serverData->buffer], params, size);
	}
	serverData->rid = methodId;
	serverData->rsize = size;

	assert(queueData->serverDataLink == 0);
	assert(queueData->serverDataStart == serverDataAddr);
	queueData->serverDataLink = serverDataAddr;

	auto thread = m_bios.GetThread(queueData->threadId);
	assert(thread->status == CIopBios::THREAD_STATUS_SLEEPING);
	m_bios.WakeupThread(queueData->threadId, true);
	m_bios.Reschedule();
}

void CSifCmd::FinishExecRequest(uint32 serverDataAddr, uint32 returnDataAddr)
{
	auto serverData = reinterpret_cast<SIFRPCSERVERDATA*>(m_ram + serverDataAddr);
	auto returnData = m_ram + returnDataAddr;
	m_sifMan.SendCallReply(serverData->serverId, returnData);
}

void CSifCmd::FinishExecCmd()
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);

	assert(moduleData->executingCmd);
	moduleData->executingCmd = false;

	auto pendingCmdBuffer = moduleData->pendingCmdBuffer;
	auto commandHeader = reinterpret_cast<const SIFCMDHEADER*>(pendingCmdBuffer);

	uint8 commandPacketSize = commandHeader->packetSize;
	assert(moduleData->pendingCmdBufferSize >= commandPacketSize);
	memmove(pendingCmdBuffer, pendingCmdBuffer + commandPacketSize, PENDING_CMD_BUFFER_SIZE - moduleData->pendingCmdBufferSize);
	moduleData->pendingCmdBufferSize -= commandPacketSize;

	if(moduleData->pendingCmdBufferSize > 0)
	{
		ProcessNextDynamicCommand();
	}
}

void CSifCmd::FinishBindRpc(uint32 clientDataAddr, uint32 serverId)
{
	auto clientData = reinterpret_cast<SIFRPCCLIENTDATA*>(m_ram + clientDataAddr);
	clientData->serverDataAddr = serverId;
	clientData->header.semaId = m_bios.CreateSemaphore(0, 1);

	int32 result = CIopBios::KERNEL_RESULT_OK;
	result = m_bios.WaitSemaphore(clientData->header.semaId);
	assert(result == CIopBios::KERNEL_RESULT_OK);

	SIFRPCBIND bindPacket;
	memset(&bindPacket, 0, sizeof(SIFRPCBIND));
	bindPacket.header.commandId = SIF_CMD_BIND;
	bindPacket.header.packetSize = sizeof(SIFRPCBIND);
	bindPacket.serverId = serverId;
	bindPacket.clientDataAddr = clientDataAddr;
	m_sifMan.SendPacket(&bindPacket, sizeof(bindPacket));
}

void CSifCmd::ProcessCustomCommand(uint32 commandHeaderAddr)
{
	auto commandHeader = reinterpret_cast<const SIFCMDHEADER*>(m_ram + commandHeaderAddr);
	switch(commandHeader->commandId)
	{
	case SIF_CMD_SETSREG:
		ProcessSetSreg(commandHeaderAddr);
		break;
	case 0x80000004:
		//No clue what this is used for, but seems to be used after "WriteToIop" is used.
		//Could be FlushCache or something like that
		break;
	case SIF_CMD_REND:
		ProcessRpcRequestEnd(commandHeaderAddr);
		break;
	default:
		ProcessDynamicCommand(commandHeaderAddr);
		break;
	}
}

void CSifCmd::ProcessSetSreg(uint32 commandHeaderAddr)
{
	auto setSreg = reinterpret_cast<const SIFSETSREG*>(m_ram + commandHeaderAddr);
	assert(setSreg->header.packetSize == sizeof(SIFSETSREG));
	assert(setSreg->index < MAX_SREG);
	if(setSreg->index >= MAX_SREG) return;
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
	moduleData->sreg[setSreg->index] = setSreg->value;
}

void CSifCmd::ProcessRpcRequestEnd(uint32 commandHeaderAddr)
{
	auto requestEnd = reinterpret_cast<const SIFRPCREQUESTEND*>(m_ram + commandHeaderAddr);
	assert(requestEnd->clientDataAddr != 0);
	auto clientData = reinterpret_cast<SIFRPCCLIENTDATA*>(m_ram + requestEnd->clientDataAddr);
	if(requestEnd->commandId == SIF_CMD_BIND)
	{
		//When serverDataAddr is 0, EE failed to find requested server ID
		assert(requestEnd->serverDataAddr != 0);
		clientData->serverDataAddr = requestEnd->serverDataAddr;
		clientData->buffPtr = requestEnd->buffer;
		clientData->cbuffPtr = requestEnd->cbuffer;
	}
	else if(requestEnd->commandId == SIF_CMD_CALL)
	{
		if(clientData->endFctPtr != 0)
		{
			m_bios.TriggerCallback(clientData->endFctPtr, clientData->endParam);
		}
	}
	else
	{
		assert(0);
	}
	//Unlock/delete semaphore
	{
		assert(clientData->header.semaId != 0);
		int32 result = CIopBios::KERNEL_RESULT_OK;
		result = m_bios.SignalSemaphore(clientData->header.semaId, true);
		assert(result == CIopBios::KERNEL_RESULT_OK);
		result = m_bios.DeleteSemaphore(clientData->header.semaId);
		assert(result == CIopBios::KERNEL_RESULT_OK);
		clientData->header.semaId = 0;
	}
}

void CSifCmd::ProcessDynamicCommand(uint32 commandHeaderAddr)
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
	auto commandHeader = reinterpret_cast<const SIFCMDHEADER*>(m_ram + commandHeaderAddr);

	uint8 commandPacketSize = commandHeader->packetSize;
	assert((moduleData->pendingCmdBufferSize + commandPacketSize) <= PENDING_CMD_BUFFER_SIZE);

	if((moduleData->pendingCmdBufferSize + commandPacketSize) <= PENDING_CMD_BUFFER_SIZE)
	{
		memcpy(moduleData->pendingCmdBuffer + moduleData->pendingCmdBufferSize, commandHeader, commandPacketSize);
		moduleData->pendingCmdBufferSize += commandPacketSize;

		if(!moduleData->executingCmd)
		{
			ProcessNextDynamicCommand();
		}
	}
}

void CSifCmd::ProcessNextDynamicCommand()
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);

	assert(!moduleData->executingCmd);
	moduleData->executingCmd = true;

	uint32 commandHeaderAddr = m_moduleDataAddr + offsetof(MODULEDATA, pendingCmdBuffer);
	auto commandHeader = reinterpret_cast<const SIFCMDHEADER*>(m_ram + commandHeaderAddr);
	bool isSystemCommand = (commandHeader->commandId & SYSTEM_COMMAND_ID) != 0;
	uint32 cmd = commandHeader->commandId & ~SYSTEM_COMMAND_ID;
	uint32 cmdBufferAddr = isSystemCommand ? m_sysCmdBufferAddr : moduleData->usrCmdBufferAddr;
	uint32 cmdBufferLen = isSystemCommand ? MAX_SYSTEM_COMMAND : moduleData->usrCmdBufferLen;

	if((cmdBufferAddr != 0) && (cmd < cmdBufferLen))
	{
		const auto& cmdDataEntry = reinterpret_cast<SIFCMDDATA*>(m_ram + cmdBufferAddr)[cmd];

		CLog::GetInstance().Print(LOG_NAME, "Calling SIF command handler for command 0x%08X at 0x%08X with data 0x%08X.\r\n",
		                          commandHeader->commandId, cmdDataEntry.sifCmdHandler, cmdDataEntry.data);

		//assert(cmdDataEntry.sifCmdHandler != 0);
		if(cmdDataEntry.sifCmdHandler != 0)
		{
			//This expects to be in an interrupt and the handler is called in the interrupt.
			//That's not the case here though, so we try for the same effect by calling the handler outside of an interrupt.
			uint32 cmdDataEntryAddr = reinterpret_cast<const uint8*>(&cmdDataEntry) - m_ram;
			m_bios.TriggerCallback(m_sifExecCmdHandlerAddr, cmdDataEntryAddr, commandHeaderAddr);
			m_bios.Reschedule();
		}
		else
		{
			FinishExecCmd();
		}
	}
	else
	{
		assert(false);
		FinishExecCmd();
	}
}

int32 CSifCmd::SifGetSreg(uint32 regIndex)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFGETSREG "(regIndex = %d);\r\n",
	                          regIndex);
	assert(regIndex < MAX_SREG);
	if(regIndex >= MAX_SREG)
	{
		return 0;
	}
	auto moduleData = reinterpret_cast<const MODULEDATA*>(m_ram + m_moduleDataAddr);
	uint32 result = moduleData->sreg[regIndex];
	return result;
}

uint32 CSifCmd::SifSetCmdBuffer(uint32 cmdBufferAddr, uint32 length)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSETCMDBUFFER "(cmdBufferAddr = 0x%08X, length = %d);\r\n",
	                          cmdBufferAddr, length);

	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
	uint32 originalBuffer = moduleData->usrCmdBufferAddr;
	moduleData->usrCmdBufferAddr = cmdBufferAddr;
	moduleData->usrCmdBufferLen = length;

	return originalBuffer;
}

void CSifCmd::SifAddCmdHandler(uint32 pos, uint32 handler, uint32 data)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFADDCMDHANDLER "(pos = 0x%08X, handler = 0x%08X, data = 0x%08X);\r\n",
	                          pos, handler, data);

	auto moduleData = reinterpret_cast<const MODULEDATA*>(m_ram + m_moduleDataAddr);
	bool isSystemCommand = (pos & SYSTEM_COMMAND_ID) != 0;
	uint32 cmd = pos & ~SYSTEM_COMMAND_ID;
	uint32 cmdBufferAddr = isSystemCommand ? m_sysCmdBufferAddr : moduleData->usrCmdBufferAddr;
	uint32 cmdBufferLen = isSystemCommand ? MAX_SYSTEM_COMMAND : moduleData->usrCmdBufferLen;

	if((cmdBufferAddr != 0) && (cmd < cmdBufferLen))
	{
		auto& cmdDataEntry = reinterpret_cast<SIFCMDDATA*>(m_ram + cmdBufferAddr)[cmd];
		cmdDataEntry.sifCmdHandler = handler;
		cmdDataEntry.data = data;
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "SifAddCmdHandler - error command buffer too small or not set.\r\n");
	}
}

uint32 CSifCmd::SifSendCmd(uint32 commandId, uint32 packetPtr, uint32 packetSize, uint32 srcExtraPtr, uint32 dstExtraPtr, uint32 sizeExtra)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSENDCMD "(commandId = 0x%08X, packetPtr = 0x%08X, packetSize = 0x%08X, srcExtraPtr = 0x%08X, dstExtraPtr = 0x%08X, sizeExtra = 0x%08X);\r\n",
	                          commandId, packetPtr, packetSize, srcExtraPtr, dstExtraPtr, sizeExtra);

	assert(packetSize >= 0x10);

	auto packetData = m_ram + packetPtr;
	auto header = reinterpret_cast<SIFCMDHEADER*>(packetData);
	header->commandId = commandId;
	header->packetSize = packetSize;
	header->destSize = 0;
	header->dest = 0;

	if(sizeExtra != 0 && srcExtraPtr != 0 && dstExtraPtr != 0)
	{
		header->destSize = sizeExtra;
		header->dest = dstExtraPtr;

		auto dmaReg = reinterpret_cast<SIFDMAREG*>(m_ram + m_sendCmdExtraStructAddr);
		dmaReg->srcAddr = srcExtraPtr;
		dmaReg->dstAddr = dstExtraPtr;
		dmaReg->size = sizeExtra;
		dmaReg->flags = 0;

		m_sifMan.SifSetDma(m_sendCmdExtraStructAddr, 1);
	}

	m_sifMan.SendPacket(packetData, packetSize);

	return 1;
}

void CSifCmd::SifBindRpc(CMIPS& context)
{
	uint32 clientDataAddr = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 serverId = context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 mode = context.m_State.nGPR[CMIPS::A2].nV0;

	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFBINDRPC "(clientDataAddr = 0x%08X, serverId = 0x%08X, mode = 0x%08X);\r\n",
	                          clientDataAddr, serverId, mode);

	//Could be in non waiting mode
	assert(mode == 0);

	context.m_State.nPC = m_sifBindRpcAddr;
}

void CSifCmd::SifCallRpc(CMIPS& context)
{
	uint32 clientDataAddr = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 rpcNumber = context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 mode = context.m_State.nGPR[CMIPS::A2].nV0;
	uint32 sendAddr = context.m_State.nGPR[CMIPS::A3].nV0;
	uint32 sendSize = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10);
	uint32 recvAddr = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14);
	uint32 recvSize = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x18);
	uint32 endFctAddr = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x1C);
	uint32 endParam = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x20);

	assert(mode == 0);

	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFCALLRPC "(clientDataAddr = 0x%08X, rpcNumber = 0x%08X, mode = 0x%08X, sendAddr = 0x%08X, sendSize = 0x%08X, "
	                                                        "recvAddr = 0x%08X, recvSize = 0x%08X, endFctAddr = 0x%08X, endParam = 0x%08X);\r\n",
	                          clientDataAddr, rpcNumber, mode, sendAddr, sendSize, recvAddr, recvSize, endFctAddr, endParam);

	auto clientData = reinterpret_cast<SIFRPCCLIENTDATA*>(m_ram + clientDataAddr);
	assert(clientData->serverDataAddr != 0);
	clientData->endFctPtr = endFctAddr;
	clientData->endParam = endParam;
	clientData->header.semaId = m_bios.CreateSemaphore(0, 1);
	int32 result = CIopBios::KERNEL_RESULT_OK;
	result = m_bios.WaitSemaphore(clientData->header.semaId);
	assert(result == CIopBios::KERNEL_RESULT_OK);

	{
		auto dmaReg = reinterpret_cast<SIFDMAREG*>(m_ram + m_sendCmdExtraStructAddr);
		dmaReg->srcAddr = sendAddr;
		dmaReg->dstAddr = clientData->buffPtr;
		dmaReg->size = sendSize;
		dmaReg->flags = 0;

		m_sifMan.SifSetDma(m_sendCmdExtraStructAddr, 1);
	}

	SIFRPCCALL callPacket;
	memset(&callPacket, 0, sizeof(SIFRPCCALL));
	callPacket.header.commandId = SIF_CMD_CALL;
	callPacket.header.packetSize = sizeof(SIFRPCCALL);
	callPacket.header.destSize = sendSize;
	callPacket.header.dest = clientData->buffPtr;
	callPacket.rpcNumber = rpcNumber;
	callPacket.sendSize = sendSize;
	callPacket.recv = recvAddr;
	callPacket.recvSize = recvSize;
	callPacket.recvMode = 1;
	callPacket.clientDataAddr = clientDataAddr;
	callPacket.serverDataAddr = clientData->serverDataAddr;

	m_sifMan.SendPacket(&callPacket, sizeof(callPacket));

	context.m_State.nGPR[CMIPS::V0].nD0 = 0;
}

void CSifCmd::SifRegisterRpc(CMIPS& context)
{
	uint32 serverDataAddr = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 serverId = context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 function = context.m_State.nGPR[CMIPS::A2].nV0;
	uint32 buffer = context.m_State.nGPR[CMIPS::A3].nV0;
	uint32 cfunction = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10);
	uint32 cbuffer = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14);
	uint32 queueAddr = context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x18);

	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFREGISTERRPC "(serverData = 0x%08X, serverId = 0x%08X, function = 0x%08X, buffer = 0x%08X, cfunction = 0x%08X, cbuffer = 0x%08X, queue = 0x%08X);\r\n",
	                          serverDataAddr, serverId, function, buffer, cfunction, cbuffer, queueAddr);

	bool moduleRegistered = m_sifMan.IsModuleRegistered(serverId);
	if(!moduleRegistered)
	{
		auto module = new CSifDynamic(*this, serverDataAddr);
		m_servers.push_back(module);
		m_sifMan.RegisterModule(serverId, module);
	}

	if(serverDataAddr != 0)
	{
		auto serverData = reinterpret_cast<SIFRPCSERVERDATA*>(&m_ram[serverDataAddr]);
		serverData->serverId = serverId;
		serverData->function = function;
		serverData->buffer = buffer;
		serverData->cfunction = cfunction;
		serverData->cbuffer = cbuffer;
		serverData->queueAddr = queueAddr;
	}

	if(queueAddr != 0)
	{
		auto queueData = reinterpret_cast<SIFRPCQUEUEDATA*>(m_ram + queueAddr);
		assert(queueData->serverDataStart == 0);
		queueData->serverDataStart = serverDataAddr;
	}

	context.m_State.nGPR[CMIPS::V0].nD0 = 0;
}

void CSifCmd::SifSetRpcQueue(uint32 queueDataAddr, uint32 threadId)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSETRPCQUEUE "(queueData = 0x%08X, threadId = %d);\r\n",
	                          queueDataAddr, threadId);

	if(queueDataAddr != 0)
	{
		auto queueData = reinterpret_cast<SIFRPCQUEUEDATA*>(m_ram + queueDataAddr);
		queueData->threadId = threadId;
		queueData->active = 0;
		queueData->serverDataLink = 0;
		queueData->serverDataStart = 0;
		queueData->serverDataEnd = 0;
		queueData->queueNext = 0;
	}
}

uint32 CSifCmd::SifGetNextRequest(uint32 queueDataAddr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFGETNEXTREQUEST "(queueData = 0x%08X);\r\n",
	                          queueDataAddr);

	uint32 result = 0;
	if(queueDataAddr != 0)
	{
		auto queueData = reinterpret_cast<SIFRPCQUEUEDATA*>(m_ram + queueDataAddr);
		result = queueData->serverDataLink;
		queueData->serverDataLink = 0;
	}
	return result;
}

void CSifCmd::SifExecRequest(CMIPS& context)
{
	uint32 serverDataAddr = context.m_State.nGPR[CMIPS::A0].nV0;
	auto serverData = reinterpret_cast<SIFRPCSERVERDATA*>(&m_ram[serverDataAddr]);
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFEXECREQUEST "(serverData = 0x%08X, serverId=0x%x, function=0x%x, rid=0x%x, buffer=0x%x, rsize=0x%x);\r\n",
	                          serverDataAddr, serverData->serverId, serverData->function, serverData->rid, serverData->buffer, serverData->rsize);
	context.m_State.nPC = m_sifExecRequestAddr;
}

uint32 CSifCmd::SifCheckStatRpc(uint32 clientDataAddress)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFCHECKSTATRPC "(clientData = 0x%08X);\r\n",
	                          clientDataAddress);
	return 0;
}

void CSifCmd::SifRpcLoop(CMIPS& context)
{
	uint32 queueAddr = context.m_State.nGPR[CMIPS::A0].nV0;
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFRPCLOOP "(queue = 0x%08X);\r\n",
	                          queueAddr);
	context.m_State.nPC = m_sifRpcLoopAddr;
}

uint32 CSifCmd::SifGetOtherData(uint32 packetPtr, uint32 src, uint32 dst, uint32 size, uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFGETOTHERDATA "(packetPtr = 0x%08X, src = 0x%08X, dst = 0x%08X, size = 0x%08X, mode = %d);\r\n",
	                          packetPtr, src, dst, size, mode);
	m_sifMan.GetOtherData(dst, src, size);
	return 0;
}

uint32 CSifCmd::SifSendCmdIntr(uint32 commandId, uint32 packetPtr, uint32 packetSize, uint32 srcExtraPtr, uint32 dstExtraPtr, uint32 sizeExtra, uint32 callbackPtr, uint32 callbackDataPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSENDCMDINTR "(commandId = 0x%08X, packetPtr = 0x%08X, packetSize = 0x%08X, srcExtraPtr = 0x%08X, dstExtraPtr = 0x%08X, sizeExtra = 0x%08X, callbackPtr = 0x%08X, callbackDataPtr = 0x%08X);\r\n",
	                          commandId, packetPtr, packetSize, srcExtraPtr, dstExtraPtr, sizeExtra, callbackPtr, callbackDataPtr);

	uint32 result = SifSendCmd(commandId, packetPtr, packetSize, srcExtraPtr, dstExtraPtr, sizeExtra);
	m_bios.TriggerCallback(callbackPtr, callbackDataPtr);
	return result;
}

void CSifCmd::SleepThread()
{
	m_bios.SleepThread();
}

void CSifCmd::DelayThread(uint32 delay)
{
	m_bios.DelayThread(delay);
}
