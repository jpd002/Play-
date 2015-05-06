#include <stdio.h>
#include "../Log.h"
#include "../Ps2Const.h"
#include "../StructCollectionStateFile.h"
#include "../iop/IopBios.h"
#include "SIF.h"
#include "lexical_cast_ex.h"

#define		RPC_RECVADDR		0xDEADBEEF
#define		SIF_RESETADDR		0		//Only works if equals to 0

#define LOG_NAME					("sif")

#define STATE_REGS_XML				("sif/regs.xml")
#define STATE_CALL_REPLIES_XML		("sif/call_replies.xml")

#define STATE_PACKET_HEADER_SIZE				("Packet_Header_Size")
#define STATE_PACKET_HEADER_DEST				("Packet_Header_Dest")
#define STATE_PACKET_HEADER_CID					("Packet_Header_CId")
#define STATE_PACKET_HEADER_OPTIONAL			("Packet_Header_Optional")

#define STATE_PACKET_CALL_RECORDID				("Packet_Call_RecordId")
#define STATE_PACKET_CALL_PACKETADDR			("Packet_Call_PacketAddr")
#define STATE_PACKET_CALL_RPCID					("Packet_Call_RpcId")
#define STATE_PACKET_CALL_CLIENTDATAADDR		("Packet_Call_ClientDataAddr")
#define STATE_PACKET_CALL_RPCNUMBER				("Packet_Call_RPCNumber")
#define STATE_PACKET_CALL_SENDSIZE				("Packet_Call_SendSize")
#define STATE_PACKET_CALL_RECV					("Packet_Call_Recv")
#define STATE_PACKET_CALL_RECVSIZE				("Packet_Call_RecvSize")
#define STATE_PACKET_CALL_RECVMODE				("Packet_Call_RecvMode")
#define STATE_PACKET_CALL_SERVERDATAADDR		("Packet_Call_ServerDataAddr")

#define STATE_PACKET_REQUEST_END_RECORDID		("Packet_Request_End_RecordId")
#define STATE_PACKET_REQUEST_END_PACKETADDR		("Packet_Request_End_PacketAddr")
#define STATE_PACKET_REQUEST_END_RPCID			("Packet_Request_End_RpcId")
#define STATE_PACKET_REQUEST_END_CLIENTDATAADDR	("Packet_Request_End_ClientDataAddr")
#define STATE_PACKET_REQUEST_END_CID			("Packet_Request_End_CId")
#define STATE_PACKET_REQUEST_END_SERVERDATAADDR	("Packet_Request_End_ServerDataAddr")
#define STATE_PACKET_REQUEST_END_BUFFER			("Packet_Request_End_Buffer")
#define STATE_PACKET_REQUEST_END_CLIENTBUFFER	("Packet_Request_End_ClientBuffer")

CSIF::CSIF(CDMAC& dmac, uint8* eeRam, uint8* iopRam)
: m_dmac(dmac)
, m_eeRam(eeRam)
, m_iopRam(iopRam)
{

}

CSIF::~CSIF()
{

}

void CSIF::Reset()
{
	m_nMAINADDR		= 0;
	m_nSUBADDR		= 0;
	m_nMSFLAG		= 0;
//	m_nSMFLAG		= 0x20000;
	m_nSMFLAG		= 0x60000;

	m_nEERecvAddr	= 0;
	m_nDataAddr		= 0;

	m_dmaBufferAddress	= 0;
	m_dmaBufferSize		= 0;
	m_cmdBufferAddress	= 0;
	m_cmdBufferSize		= 0;

	memset(m_nUserReg, 0, sizeof(uint32) * MAX_USERREG);

	m_packetQueue.clear();
	m_packetProcessed = true;

	m_callReplies.clear();
	m_bindReplies.clear();

	DeleteModules();
}

void CSIF::SetDmaBuffer(uint32 bufferAddress, uint32 size)
{
	m_dmaBufferAddress = bufferAddress;
	m_dmaBufferSize = size;
}

void CSIF::SetCmdBuffer(uint32 bufferAddress, uint32 size)
{
	m_cmdBufferAddress = bufferAddress;
	m_cmdBufferSize = size;
	m_nSUBADDR = bufferAddress;
}

void CSIF::RegisterModule(uint32 moduleId, CSifModule* module)
{
	m_modules[moduleId] = module;

	auto replyIterator(m_bindReplies.find(moduleId));
	if(replyIterator != m_bindReplies.end())
	{
		SendPacket(&(replyIterator->second), sizeof(SIFRPCREQUESTEND));
		m_bindReplies.erase(replyIterator);
	}
}

bool CSIF::IsModuleRegistered(uint32 moduleId) const
{
	return m_modules.find(moduleId) != std::end(m_modules);
}

void CSIF::UnregisterModule(uint32 moduleId)
{
	m_modules.erase(moduleId);
}

void CSIF::DeleteModules()
{
	m_modules.clear();
}

uint32 CSIF::ReceiveDMA5(uint32 srcAddress, uint32 size, uint32 unused, bool isTagIncluded)
{
	if(size > m_dmaBufferSize)
	{
		throw std::runtime_error("Packet too big.");
	}
	memcpy(m_eeRam + srcAddress, m_iopRam + m_dmaBufferAddress, size);
	return size;
}

uint32 CSIF::ReceiveDMA6(uint32 nSrcAddr, uint32 nSize, uint32 nDstAddr, bool isTagIncluded)
{
	assert(!isTagIncluded);

	//Humm, this is kinda odd, but it ors the address with 0x20000000
	nSrcAddr &= (PS2::EE_RAM_SIZE - 1);

	if(nDstAddr == RPC_RECVADDR)
	{
		//This should be the arguments for the call command
		//Just save the source address for later use
		m_nDataAddr = nSrcAddr;
		return nSize;
	}
	else if(nDstAddr == SIF_RESETADDR)
	{
		auto commandData = m_eeRam + nSrcAddr;
		if(commandData[0] == 0x68)
		{
			auto pathSize = *reinterpret_cast<uint32*>(commandData + 0x10);
			auto path = std::string(commandData + 0x18, commandData + 0x18 + pathSize);
			if(m_moduleResetHandler)
			{
				m_moduleResetHandler(path);
			}
		}
		return nSize;
	}
	else if(nDstAddr == m_nSUBADDR)
	{
		auto hdr = reinterpret_cast<SIFCMDHEADER*>(m_eeRam + nSrcAddr);

		//This is kinda odd...
		//plasma_tunnel.elf does this
/*
	    if((hdr->nCID & 0xFF000000) == 0x08000000)
	    {
		    hdr->nCID &= 0x00FFFFFF;
		    hdr->nCID |= 0x80000000;
	    }
*/
		CLog::GetInstance().Print(LOG_NAME, "Received command 0x%0.8X.\r\n", hdr->commandId);

		switch(hdr->commandId)
		{
		case 0x80000000:
			Cmd_SetEERecvAddr(hdr);
			break;
		case SIF_CMD_INIT:
			Cmd_Initialize(hdr);
			break;
		case SIF_CMD_BIND:
			Cmd_Bind(hdr);
			break;
		case SIF_CMD_CALL:
			Cmd_Call(hdr);
			break;
		case SIF_CMD_OTHERDATA:
			Cmd_GetOtherData(hdr);
			break;
		default:
			if(m_customCommandHandler)
			{
				memcpy(m_iopRam + nDstAddr, m_eeRam + nSrcAddr, nSize);
				m_customCommandHandler(nDstAddr);
			}
			break;
		}

		return nSize;
	}
	else
	{
		assert(nDstAddr < PS2::IOP_RAM_SIZE);
		CLog::GetInstance().Print(LOG_NAME, "WriteToIop(dstAddr = 0x%0.8X, srcAddr = 0x%0.8X, size = 0x%0.8X);\r\n", 
			nDstAddr, nSrcAddr, nSize);
		if(nDstAddr >= 0 && nDstAddr <= CIopBios::CONTROL_BLOCK_END)
		{
			CLog::GetInstance().Print(LOG_NAME, "Warning: Trying to DMA in Bios Control Area.\r\n");
		}
		else
		{
			memcpy(m_iopRam + nDstAddr, m_eeRam + nSrcAddr, nSize);
		}
		return nSize;
	}
}

void CSIF::SendPacket(void* packet, uint32 size)
{
	m_packetQueue.insert(m_packetQueue.begin(), 
		reinterpret_cast<uint8*>(packet), 
		reinterpret_cast<uint8*>(packet) + size);
	m_packetQueue.insert(m_packetQueue.begin(),
		reinterpret_cast<uint8*>(&size),
		reinterpret_cast<uint8*>(&size) + 4);
}

void CSIF::ProcessPackets()
{
	if(m_packetProcessed && !m_packetQueue.empty())
	{
		assert(m_packetQueue.size() > 4);
		uint32 size = *reinterpret_cast<uint32*>(&m_packetQueue[0]);
		SendDMA(&m_packetQueue[4], size);
		m_packetQueue.erase(m_packetQueue.begin(), m_packetQueue.begin() + 4 + size);
		m_packetProcessed = false;
	}
}

void CSIF::MarkPacketProcessed()
{
	assert(m_packetProcessed == false);
	m_packetProcessed = true;
}

void CSIF::SendDMA(void* pData, uint32 nSize)
{
	//Humm, the DMAC doesn't know about our addresses on this side...

	if(nSize > m_dmaBufferSize)
	{
		throw std::runtime_error("Packet too big.");
	}

	memcpy(m_iopRam + m_dmaBufferAddress, pData, nSize);
	uint32 nQuads = (nSize + 0x0F) / 0x10;

	m_dmac.SetRegister(CDMAC::D5_MADR, m_nEERecvAddr);
	m_dmac.SetRegister(CDMAC::D5_QWC,  nQuads);
	m_dmac.SetRegister(CDMAC::D5_CHCR, 0x00000100);
}

void CSIF::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_nMAINADDR		= registerFile.GetRegister32("MAINADDR");
	m_nSUBADDR		= registerFile.GetRegister32("SUBADDR");
	m_nMSFLAG		= registerFile.GetRegister32("MSFLAG");
	m_nSMFLAG		= registerFile.GetRegister32("SMFLAG");
	m_nEERecvAddr	= registerFile.GetRegister32("EERecvAddr");
	m_nDataAddr		= registerFile.GetRegister32("DataAddr");

	{
		CStructCollectionStateFile callRepliesFile(*archive.BeginReadFile(STATE_CALL_REPLIES_XML));
		for(CStructCollectionStateFile::StructIterator callReplyIterator(callRepliesFile.GetStructBegin());
			callReplyIterator != callRepliesFile.GetStructEnd(); callReplyIterator++)
		{
			const CStructFile& structFile(callReplyIterator->second);
			uint32 replyId = lexical_cast_hex<std::string>(callReplyIterator->first);
			CALLREQUESTINFO callReply;
			LoadState_RpcCall(structFile, callReply.call); 
			LoadState_RequestEnd(structFile, callReply.reply);
			m_callReplies[replyId] = callReply;
		}
	}
}

void CSIF::SaveState(Framework::CZipArchiveWriter& archive)
{
	{
		CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
		registerFile->SetRegister32("MAINADDR",		m_nMAINADDR);
		registerFile->SetRegister32("SUBADDR",		m_nSUBADDR);
		registerFile->SetRegister32("MSFLAG",		m_nMSFLAG);
		registerFile->SetRegister32("SMFLAG",		m_nSMFLAG);
		registerFile->SetRegister32("EERecvAddr",	m_nEERecvAddr);
		registerFile->SetRegister32("DataAddr",		m_nDataAddr);
		archive.InsertFile(registerFile);
	}

	{
		CStructCollectionStateFile* callRepliesFile = new CStructCollectionStateFile(STATE_CALL_REPLIES_XML);
		for(CallReplyMap::const_iterator callReplyIterator(m_callReplies.begin());
			callReplyIterator != m_callReplies.end(); callReplyIterator++)
		{
			const CALLREQUESTINFO& callReply(callReplyIterator->second);
			std::string replyId = lexical_cast_hex<std::string>(callReplyIterator->first, 8);
			CStructFile replyStruct;
			{
				SaveState_RpcCall(replyStruct, callReply.call);
				SaveState_RequestEnd(replyStruct, callReply.reply);
			}
			callRepliesFile->InsertStruct(replyId.c_str(), replyStruct);
		}
		archive.InsertFile(callRepliesFile);
	}
}

void CSIF::SaveState_Header(const std::string& prefix, CStructFile& file, const SIFCMDHEADER& packetHeader)
{
	file.SetRegister32((prefix + STATE_PACKET_HEADER_SIZE).c_str(),			packetHeader.size);
	file.SetRegister32((prefix + STATE_PACKET_HEADER_DEST).c_str(),			packetHeader.dest);
	file.SetRegister32((prefix + STATE_PACKET_HEADER_CID).c_str(),			packetHeader.commandId);
	file.SetRegister32((prefix + STATE_PACKET_HEADER_OPTIONAL).c_str(),		packetHeader.optional);
}

void CSIF::LoadState_Header(const std::string& prefix, const CStructFile& file, SIFCMDHEADER& packetHeader)
{
	packetHeader.size		= file.GetRegister32((prefix + STATE_PACKET_HEADER_SIZE).c_str());
	packetHeader.dest		= file.GetRegister32((prefix + STATE_PACKET_HEADER_DEST).c_str());
	packetHeader.commandId	= file.GetRegister32((prefix + STATE_PACKET_HEADER_CID).c_str());
	packetHeader.optional	= file.GetRegister32((prefix + STATE_PACKET_HEADER_OPTIONAL).c_str());
}

void CSIF::SaveState_RpcCall(CStructFile& file, const SIFRPCCALL& call)
{
	SaveState_Header("call", file, call.header);
	file.SetRegister32(STATE_PACKET_CALL_RECORDID,			call.recordId);
	file.SetRegister32(STATE_PACKET_CALL_PACKETADDR,		call.packetAddr);
	file.SetRegister32(STATE_PACKET_CALL_RPCID,				call.rpcId);
	file.SetRegister32(STATE_PACKET_CALL_CLIENTDATAADDR,	call.clientDataAddr);
	file.SetRegister32(STATE_PACKET_CALL_RPCNUMBER,			call.rpcNumber);
	file.SetRegister32(STATE_PACKET_CALL_SENDSIZE,			call.sendSize);
	file.SetRegister32(STATE_PACKET_CALL_RECV,				call.recv);
	file.SetRegister32(STATE_PACKET_CALL_RECVSIZE,			call.recvSize);
	file.SetRegister32(STATE_PACKET_CALL_RECVMODE,			call.recvMode);
	file.SetRegister32(STATE_PACKET_CALL_SERVERDATAADDR,	call.serverDataAddr);
}

void CSIF::LoadState_RpcCall(const CStructFile& file, SIFRPCCALL& call)
{
	LoadState_Header("call", file, call.header);
	call.recordId			= file.GetRegister32(STATE_PACKET_CALL_RECORDID);
	call.packetAddr			= file.GetRegister32(STATE_PACKET_CALL_PACKETADDR);
	call.rpcId				= file.GetRegister32(STATE_PACKET_CALL_RPCID);
	call.clientDataAddr		= file.GetRegister32(STATE_PACKET_CALL_CLIENTDATAADDR);
	call.rpcNumber			= file.GetRegister32(STATE_PACKET_CALL_RPCNUMBER);
	call.sendSize			= file.GetRegister32(STATE_PACKET_CALL_SENDSIZE);
	call.recv				= file.GetRegister32(STATE_PACKET_CALL_RECV);
	call.recvSize			= file.GetRegister32(STATE_PACKET_CALL_RECVSIZE);
	call.recvMode			= file.GetRegister32(STATE_PACKET_CALL_RECVMODE);
	call.serverDataAddr		= file.GetRegister32(STATE_PACKET_CALL_SERVERDATAADDR);
}

void CSIF::SaveState_RequestEnd(CStructFile& file, const SIFRPCREQUESTEND& requestEnd)
{
	SaveState_Header("requestEnd", file, requestEnd.header);
	file.SetRegister32(STATE_PACKET_REQUEST_END_RECORDID,		requestEnd.recordId);
	file.SetRegister32(STATE_PACKET_REQUEST_END_PACKETADDR,		requestEnd.packetAddr);
	file.SetRegister32(STATE_PACKET_REQUEST_END_RPCID,			requestEnd.rpcId);
	file.SetRegister32(STATE_PACKET_REQUEST_END_CLIENTDATAADDR,	requestEnd.clientDataAddr);
	file.SetRegister32(STATE_PACKET_REQUEST_END_CID,			requestEnd.commandId);
	file.SetRegister32(STATE_PACKET_REQUEST_END_SERVERDATAADDR,	requestEnd.serverDataAddr);
	file.SetRegister32(STATE_PACKET_REQUEST_END_BUFFER,			requestEnd.buffer);
	file.SetRegister32(STATE_PACKET_REQUEST_END_CLIENTBUFFER,	requestEnd.cbuffer);
}

void CSIF::LoadState_RequestEnd(const CStructFile& file, SIFRPCREQUESTEND& requestEnd)
{
	LoadState_Header("requestEnd", file, requestEnd.header);
	requestEnd.recordId			= file.GetRegister32(STATE_PACKET_REQUEST_END_RECORDID);
	requestEnd.packetAddr		= file.GetRegister32(STATE_PACKET_REQUEST_END_PACKETADDR);
	requestEnd.rpcId			= file.GetRegister32(STATE_PACKET_REQUEST_END_RPCID);
	requestEnd.clientDataAddr	= file.GetRegister32(STATE_PACKET_REQUEST_END_CLIENTDATAADDR);
	requestEnd.commandId		= file.GetRegister32(STATE_PACKET_REQUEST_END_CID);
	requestEnd.serverDataAddr	= file.GetRegister32(STATE_PACKET_REQUEST_END_SERVERDATAADDR);
	requestEnd.buffer			= file.GetRegister32(STATE_PACKET_REQUEST_END_BUFFER);
	requestEnd.cbuffer			= file.GetRegister32(STATE_PACKET_REQUEST_END_CLIENTBUFFER);
}

/////////////////////////////////////////////////////////
//SIF Commands
/////////////////////////////////////////////////////////

void CSIF::Cmd_SetEERecvAddr(SIFCMDHEADER* hdr)
{
	assert(0);
}

void CSIF::Cmd_Initialize(SIFCMDHEADER* hdr)
{
	struct INIT
	{
		SIFCMDHEADER	Header;
		uint32			nEEAddress;
	};

	INIT* pInit = reinterpret_cast<INIT*>(hdr);

	if(pInit->Header.optional == 0)
	{
		m_nEERecvAddr =  pInit->nEEAddress;
		m_nEERecvAddr &= (PS2::EE_RAM_SIZE - 1);
	}
	else if(pInit->Header.optional == 1)
	{
		//If this is set to 1, and we need to disregard the address received and send a command back...
		//Not sure about this though
		SETSREG SReg;
		SReg.Header.commandId	= 0x80000001;
		SReg.Header.size		= sizeof(SETSREG);
		SReg.Header.dest		= NULL;
		SReg.Header.optional	= 0;
		SReg.nRegister			= 0;
		SReg.nValue				= 1;

		SendPacket(&SReg, sizeof(SETSREG));
		//SendDMA(&SReg, sizeof(SETSREG));
	}
	else
	{
		assert(0);
	}
}

void CSIF::Cmd_Bind(SIFCMDHEADER* hdr)
{
	auto bind = reinterpret_cast<SIFRPCBIND*>(hdr);

	SIFRPCREQUESTEND rend;
	memset(&rend, 0, sizeof(SIFRPCREQUESTEND));
	rend.header.size		= sizeof(SIFRPCREQUESTEND);
	rend.header.dest		= hdr->dest;
	rend.header.commandId	= SIF_CMD_REND;
	rend.header.optional	= 0;
	rend.recordId			= bind->recordId;
	rend.packetAddr			= bind->packetAddr;
	rend.rpcId				= bind->rpcId;
	rend.clientDataAddr		= bind->clientDataAddr;
	rend.commandId			= SIF_CMD_BIND;
	rend.serverDataAddr		= bind->serverId;
	rend.buffer				= RPC_RECVADDR;
	rend.cbuffer			= 0xDEADCAFE;

	CLog::GetInstance().Print(LOG_NAME, "Bound client data (0x%0.8X) with server id 0x%0.8X.\r\n", bind->clientDataAddr, bind->serverId);

	auto moduleIterator(m_modules.find(bind->serverId));
	if(moduleIterator != m_modules.end() || (bind->serverId & 0x80000000) != 0)
	{
		SendPacket(&rend, sizeof(SIFRPCREQUESTEND));
	}
	else
	{
		assert(m_bindReplies.find(bind->serverId) == m_bindReplies.end());
		m_bindReplies[bind->serverId] = rend;
	}
}

void CSIF::Cmd_Call(SIFCMDHEADER* hdr)
{
	auto call = reinterpret_cast<SIFRPCCALL*>(hdr);
	bool sendReply = true;

	CLog::GetInstance().Print(LOG_NAME, "Calling function 0x%0.8X of module 0x%0.8X.\r\n", call->rpcNumber, call->serverDataAddr);

	uint32 nRecvAddr = (call->recv & (PS2::EE_RAM_SIZE - 1));

	auto moduleIterator(m_modules.find(call->serverDataAddr));
	if(moduleIterator != std::end(m_modules))
	{
		CSifModule* pModule(moduleIterator->second);
		sendReply = pModule->Invoke(call->rpcNumber, 
			reinterpret_cast<uint32*>(m_eeRam + m_nDataAddr), call->sendSize, 
			reinterpret_cast<uint32*>(m_eeRam + nRecvAddr), call->recvSize,
			m_eeRam);
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Called an unknown module (0x%0.8X).\r\n", call->serverDataAddr);
	}

	{
		SIFRPCREQUESTEND rend;
		memset(&rend, 0, sizeof(SIFRPCREQUESTEND));
		rend.header.size		= sizeof(SIFRPCREQUESTEND);
		rend.header.dest		= hdr->dest;
		rend.header.commandId	= SIF_CMD_REND;
		rend.header.optional	= 0;
		rend.recordId			= call->recordId;
		rend.packetAddr			= call->packetAddr;
		rend.rpcId				= call->rpcId;
		rend.clientDataAddr		= call->clientDataAddr;
		rend.commandId			= SIF_CMD_CALL;

		if(sendReply)
		{
			SendPacket(&rend, sizeof(SIFRPCREQUESTEND));
		}
		else
		{
			//Hold the packet
			//We assume that there's only one call that
			assert(m_callReplies.find(call->serverDataAddr) == m_callReplies.end());
			CALLREQUESTINFO requestInfo;
			requestInfo.reply = rend;
			requestInfo.call = *call;
			m_callReplies[call->serverDataAddr] = requestInfo;
		}
	}
}

void CSIF::Cmd_GetOtherData(SIFCMDHEADER* hdr)
{
	auto otherData = reinterpret_cast<SIFRPCOTHERDATA*>(hdr);

	uint32 dstPtr = otherData->dstPtr & (PS2::EE_RAM_SIZE - 1);
	uint32 srcPtr = otherData->srcPtr & (PS2::IOP_RAM_SIZE - 1);

	memcpy(m_eeRam + dstPtr, m_iopRam + srcPtr, otherData->size);

	{
		SIFRPCREQUESTEND rend;
		memset(&rend, 0, sizeof(SIFRPCREQUESTEND));
		rend.header.size		= sizeof(SIFRPCREQUESTEND);
		rend.header.dest		= hdr->dest;
		rend.header.commandId	= SIF_CMD_REND;
		rend.header.optional	= 0;
		rend.recordId			= otherData->recordId;
		rend.packetAddr			= otherData->packetAddr;
		rend.rpcId				= otherData->rpcId;
		rend.clientDataAddr		= otherData->receiveDataAddr;
		rend.commandId			= SIF_CMD_OTHERDATA;

		SendPacket(&rend, sizeof(SIFRPCREQUESTEND));
	}
}

void CSIF::SendCallReply(uint32 serverId, const void* returnData)
{
	auto replyIterator(m_callReplies.find(serverId));
	assert(replyIterator != m_callReplies.end());
	if(replyIterator == m_callReplies.end()) return;

	auto& requestInfo(replyIterator->second);
	if(requestInfo.call.recv != 0 && returnData != nullptr)
	{
		uint32 dstPtr = requestInfo.call.recv & (PS2::EE_RAM_SIZE - 1);
		memcpy(m_eeRam + dstPtr, returnData, requestInfo.call.recvSize);
	}
	SendPacket(&requestInfo.reply, sizeof(SIFRPCREQUESTEND));
	m_callReplies.erase(replyIterator);
}

void CSIF::SetModuleResetHandler(const ModuleResetHandler& moduleResetHandler)
{
	m_moduleResetHandler = moduleResetHandler;
}

void CSIF::SetCustomCommandHandler(const CustomCommandHandler& customCommandHandler)
{
	m_customCommandHandler = customCommandHandler;
}

/////////////////////////////////////////////////////////
//Get/Set Register
/////////////////////////////////////////////////////////

uint32 CSIF::GetRegister(uint32 nRegister)
{
/*
	if(nRegister & 0x80000000)
	{
		nRegister &= ~0x80000000;
		if(nRegister >= MAX_USERREG)
		{
			printf("SIF: Warning. Trying to read an unimplemented user register (0x%0.8X).\r\n", nRegister | 0x80000000);
			return 0;
		}
		return m_nUserReg[nRegister];
	}
	else
*/
	{
		switch(nRegister)
		{
		case 0x00000001:
			return m_nMAINADDR;
			break;
		case 0x00000002:
			return m_nSUBADDR;
			break;
		case 0x00000003:
			return m_nMSFLAG;
			break;
		case 0x00000004:
			return m_nSMFLAG;
			break;
		case 0x80000000:
			return SIF_RESETADDR;
			break;
		case 0x80000002:
//			return 0;
			return 1;
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Warning. Trying to read an invalid system register (0x%0.8X).\r\n", nRegister);
			return 0;
			break;
		}
	}
}

void CSIF::SetRegister(uint32 nRegister, uint32 nValue)
{
/*
	if(nRegister & 0x80000000)
	{
		nRegister &= ~0x80000000;
		if(nRegister >= MAX_USERREG)
		{
			printf("SIF: Warning. Trying to write to an unimplemented user register (0x%0.8X).\r\n", nRegister | 0x80000000);
			return;
		}
		m_nUserReg[nRegister] = nValue;
	}
	else*/
	{
		switch(nRegister)
		{
		case 0x00000001:
			m_nMAINADDR = nValue;
			break;
		case 0x80000000:
			//Set by CMD library
			break;
		case 0x80000001:
			//Set by CMD library
			break;
		case 0x80000002:
			//Set by RPC library (initialized state?)
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Warning. Trying to write to an invalid system register (0x%0.8X).\r\n", nRegister);
			break;
		}
	}
}
