#include <stdio.h>
#include "../Log.h"
#include "../Ps2Const.h"
#include "../StructCollectionStateFile.h"
#include "../MemoryStateFile.h"
#include "../iop/IopBios.h"
#include "SIF.h"
#include "lexical_cast_ex.h"
#include "string_format.h"

#define		RPC_RECVADDR		0xDEADBEF0
#define		SIF_RESETADDR		0		//Only works if equals to 0

#define LOG_NAME					("sif")

#define STATE_REGS_XML				("sif/regs.xml")
#define STATE_PACKETQUEUE			("sif/packet_queue")
#define STATE_CALL_REPLIES_XML		("sif/call_replies.xml")
#define STATE_BIND_REPLIES_XML		("sif/bind_replies.xml")

#define STATE_REG_MAINADDR        ("MAINADDR")
#define STATE_REG_SUBADDR         ("SUBADDR")
#define STATE_REG_MSFLAG          ("MSFLAG")
#define STATE_REG_SMFLAG          ("SMFLAG")
#define STATE_REG_EERECVADDR      ("EERecvAddr")
#define STATE_REG_DATAADDR        ("DataAddr")
#define STATE_REG_PACKETPROCESSED ("packetProcessed")

#define STATE_PACKET_HEADER_PACKETSIZE			("Packet_Header_PacketSize")
#define STATE_PACKET_HEADER_DESTSIZE			("Packet_Header_DestSize")
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

		CLog::GetInstance().Print(LOG_NAME, "Received command 0x%08X.\r\n", hdr->commandId);

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
		CLog::GetInstance().Print(LOG_NAME, "WriteToIop(dstAddr = 0x%08X, srcAddr = 0x%08X, size = 0x%08X);\r\n", 
			nDstAddr, nSrcAddr, nSize);
		nSize &= 0x7FFFFFFF;		//Fix for Gregory Horror Show's crash
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
	{
		auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_REGS_XML));
		m_nMAINADDR       = registerFile.GetRegister32(STATE_REG_MAINADDR);
		m_nSUBADDR        = registerFile.GetRegister32(STATE_REG_SUBADDR);
		m_nMSFLAG         = registerFile.GetRegister32(STATE_REG_MSFLAG);
		m_nSMFLAG         = registerFile.GetRegister32(STATE_REG_SMFLAG);
		m_nEERecvAddr     = registerFile.GetRegister32(STATE_REG_EERECVADDR);
		m_nDataAddr       = registerFile.GetRegister32(STATE_REG_DATAADDR);
		m_packetProcessed = registerFile.GetRegister32(STATE_REG_PACKETPROCESSED) != 0;
	}

	m_packetQueue = LoadPacketQueue(archive);

	m_callReplies = LoadCallReplies(archive);
	m_bindReplies = LoadBindReplies(archive);
}

void CSIF::SaveState(Framework::CZipArchiveWriter& archive)
{
	{
		auto registerFile = new CRegisterStateFile(STATE_REGS_XML);
		registerFile->SetRegister32(STATE_REG_MAINADDR,        m_nMAINADDR);
		registerFile->SetRegister32(STATE_REG_SUBADDR,         m_nSUBADDR);
		registerFile->SetRegister32(STATE_REG_MSFLAG,          m_nMSFLAG);
		registerFile->SetRegister32(STATE_REG_SMFLAG,          m_nSMFLAG);
		registerFile->SetRegister32(STATE_REG_EERECVADDR,      m_nEERecvAddr);
		registerFile->SetRegister32(STATE_REG_DATAADDR,        m_nDataAddr);
		registerFile->SetRegister32(STATE_REG_PACKETPROCESSED, m_packetProcessed);
		archive.InsertFile(registerFile);
	}

	archive.InsertFile(new CMemoryStateFile(STATE_PACKETQUEUE, m_packetQueue.data(), m_packetQueue.size()));

	SaveCallReplies(archive);
	SaveBindReplies(archive);
}

void CSIF::SaveCallReplies(Framework::CZipArchiveWriter& archive)
{
	auto callRepliesFile = new CStructCollectionStateFile(STATE_CALL_REPLIES_XML);
	for(const auto& callReplyIterator : m_callReplies)
	{
		const auto& callReply(callReplyIterator.second);
		auto replyId = string_format("%08x", callReplyIterator.first);
		CStructFile replyStruct;
		{
			SaveState_RpcCall(replyStruct, callReply.call);
			SaveState_RequestEnd(replyStruct, callReply.reply);
		}
		callRepliesFile->InsertStruct(replyId.c_str(), replyStruct);
	}
	archive.InsertFile(callRepliesFile);
}

void CSIF::SaveBindReplies(Framework::CZipArchiveWriter& archive)
{
	auto bindRepliesFile = new CStructCollectionStateFile(STATE_BIND_REPLIES_XML);
	for(const auto& bindReplyIterator : m_bindReplies)
	{
		const auto& bindReply(bindReplyIterator.second);
		auto replyId = string_format("%08x", bindReplyIterator.first);
		CStructFile replyStruct;
		{
			SaveState_RequestEnd(replyStruct, bindReply);
		}
		bindRepliesFile->InsertStruct(replyId.c_str(), replyStruct);
	}
	archive.InsertFile(bindRepliesFile);
}

CSIF::PacketQueue CSIF::LoadPacketQueue(Framework::CZipArchiveReader& archive)
{
	PacketQueue packetQueue;
	auto file = archive.BeginReadFile(STATE_PACKETQUEUE);
	while(1)
	{
		static const uint32 bufferSize = 0x100;
		uint8 buffer[bufferSize];
		auto readSize = file->Read(buffer, bufferSize);
		if(readSize == 0) break;
		packetQueue.insert(std::end(packetQueue), buffer, buffer + readSize);
	}
	return packetQueue;
}

CSIF::CallReplyMap CSIF::LoadCallReplies(Framework::CZipArchiveReader& archive)
{
	CallReplyMap callReplies;
	auto callRepliesFile = CStructCollectionStateFile(*archive.BeginReadFile(STATE_CALL_REPLIES_XML));
	for(const auto& structFilePair : callRepliesFile)
	{
		const auto& structFile(structFilePair.second);
		uint32 replyId = lexical_cast_hex<std::string>(structFilePair.first);
		CALLREQUESTINFO callReply;
		LoadState_RpcCall(structFile, callReply.call); 
		LoadState_RequestEnd(structFile, callReply.reply);
		callReplies[replyId] = callReply;
	}
	return callReplies;
}

CSIF::BindReplyMap CSIF::LoadBindReplies(Framework::CZipArchiveReader& archive)
{
	BindReplyMap bindReplies;
	auto bindRepliesFile = CStructCollectionStateFile(*archive.BeginReadFile(STATE_BIND_REPLIES_XML));
	for(const auto& structFilePair : bindRepliesFile)
	{
		const auto& structFile(structFilePair.second);
		uint32 replyId = lexical_cast_hex<std::string>(structFilePair.first);
		SIFRPCREQUESTEND bindReply;
		LoadState_RequestEnd(structFile, bindReply);
		bindReplies[replyId] = bindReply;
	}
	return bindReplies;
}

void CSIF::SaveState_Header(const std::string& prefix, CStructFile& file, const SIFCMDHEADER& packetHeader)
{
	file.SetRegister32((prefix + STATE_PACKET_HEADER_PACKETSIZE).c_str(), packetHeader.packetSize);
	file.SetRegister32((prefix + STATE_PACKET_HEADER_DESTSIZE).c_str(),   packetHeader.destSize);
	file.SetRegister32((prefix + STATE_PACKET_HEADER_DEST).c_str(),       packetHeader.dest);
	file.SetRegister32((prefix + STATE_PACKET_HEADER_CID).c_str(),        packetHeader.commandId);
	file.SetRegister32((prefix + STATE_PACKET_HEADER_OPTIONAL).c_str(),   packetHeader.optional);
}

void CSIF::LoadState_Header(const std::string& prefix, const CStructFile& file, SIFCMDHEADER& packetHeader)
{
	packetHeader.packetSize = file.GetRegister32((prefix + STATE_PACKET_HEADER_PACKETSIZE).c_str());
	packetHeader.destSize   = file.GetRegister32((prefix + STATE_PACKET_HEADER_DESTSIZE).c_str());
	packetHeader.dest       = file.GetRegister32((prefix + STATE_PACKET_HEADER_DEST).c_str());
	packetHeader.commandId  = file.GetRegister32((prefix + STATE_PACKET_HEADER_CID).c_str());
	packetHeader.optional   = file.GetRegister32((prefix + STATE_PACKET_HEADER_OPTIONAL).c_str());
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

void CSIF::Cmd_SetEERecvAddr(const SIFCMDHEADER* hdr)
{
	assert(0);
}

void CSIF::Cmd_Initialize(const SIFCMDHEADER* hdr)
{
	struct INIT
	{
		SIFCMDHEADER	Header;
		uint32			nEEAddress;
	};

	auto init = reinterpret_cast<const INIT*>(hdr);
	if(init->Header.optional == 0)
	{
		m_nEERecvAddr =  init->nEEAddress;
		m_nEERecvAddr &= (PS2::EE_RAM_SIZE - 1);
	}
	else if(init->Header.optional == 1)
	{
		//If 'optional' is set to 1, and we need to disregard the address received and send a command back...
		//Not sure about this though (seems to be used by SifInitRpc)

		SIFSETSREG sreg;
		memset(&sreg, 0, sizeof(SIFSETSREG));
		sreg.header.commandId  = SIF_CMD_SETSREG;
		sreg.header.packetSize = sizeof(SIFSETSREG);
		sreg.index             = 0;    //Should be SIF_SREG_RPCINIT
		sreg.value             = 1;

		SendPacket(&sreg, sizeof(SIFSETSREG));
	}
	else
	{
		assert(0);
	}
}

void CSIF::Cmd_Bind(const SIFCMDHEADER* hdr)
{
	auto bind = reinterpret_cast<const SIFRPCBIND*>(hdr);

	SIFRPCREQUESTEND rend;
	memset(&rend, 0, sizeof(SIFRPCREQUESTEND));
	rend.header.packetSize  = sizeof(SIFRPCREQUESTEND);
	rend.header.dest        = hdr->dest;
	rend.header.commandId   = SIF_CMD_REND;
	rend.header.optional    = 0;
	rend.recordId           = bind->recordId;
	rend.packetAddr         = bind->packetAddr;
	rend.rpcId              = bind->rpcId;
	rend.clientDataAddr     = bind->clientDataAddr;
	rend.commandId          = SIF_CMD_BIND;
	rend.serverDataAddr     = bind->serverId;
	rend.buffer             = RPC_RECVADDR;
	rend.cbuffer            = 0xDEADCAFE;

	CLog::GetInstance().Print(LOG_NAME, "Bound client data (0x%08X) with server id 0x%08X.\r\n", bind->clientDataAddr, bind->serverId);

	auto moduleIterator(m_modules.find(bind->serverId));
	if(moduleIterator != m_modules.end())
	{
		SendPacket(&rend, sizeof(SIFRPCREQUESTEND));
	}
	else
	{
		assert(m_bindReplies.find(bind->serverId) == m_bindReplies.end());
		m_bindReplies[bind->serverId] = rend;
	}
}

void CSIF::Cmd_Call(const SIFCMDHEADER* hdr)
{
	auto call = reinterpret_cast<const SIFRPCCALL*>(hdr);
	bool sendReply = true;

	CLog::GetInstance().Print(LOG_NAME, "Calling function 0x%08X of module 0x%08X.\r\n", call->rpcNumber, call->serverDataAddr);

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
		CLog::GetInstance().Print(LOG_NAME, "Called an unknown module (0x%08X).\r\n", call->serverDataAddr);
	}

	{
		SIFRPCREQUESTEND rend;
		memset(&rend, 0, sizeof(SIFRPCREQUESTEND));
		rend.header.packetSize  = sizeof(SIFRPCREQUESTEND);
		rend.header.dest        = hdr->dest;
		rend.header.commandId   = SIF_CMD_REND;
		rend.header.optional    = 0;
		rend.recordId           = call->recordId;
		rend.packetAddr         = call->packetAddr;
		rend.rpcId              = call->rpcId;
		rend.clientDataAddr     = call->clientDataAddr;
		rend.commandId          = SIF_CMD_CALL;

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

void CSIF::Cmd_GetOtherData(const SIFCMDHEADER* hdr)
{
	auto otherData = reinterpret_cast<const SIFRPCOTHERDATA*>(hdr);

	CLog::GetInstance().Print(LOG_NAME, "GetOtherData(dstPtr = 0x%08X, srcPtr = 0x%08X, size = 0x%08X);\r\n",
		otherData->dstPtr, otherData->srcPtr, otherData->size);

	uint32 dstPtr = otherData->dstPtr & (PS2::EE_RAM_SIZE - 1);
	uint32 srcPtr = otherData->srcPtr & (PS2::IOP_RAM_SIZE - 1);

	memcpy(m_eeRam + dstPtr, m_iopRam + srcPtr, otherData->size);

	{
		SIFRPCREQUESTEND rend;
		memset(&rend, 0, sizeof(SIFRPCREQUESTEND));
		rend.header.packetSize  = sizeof(SIFRPCREQUESTEND);
		rend.header.dest        = hdr->dest;
		rend.header.commandId   = SIF_CMD_REND;
		rend.header.optional    = 0;
		rend.recordId           = otherData->recordId;
		rend.packetAddr         = otherData->packetAddr;
		rend.rpcId              = otherData->rpcId;
		rend.clientDataAddr     = otherData->receiveDataAddr;
		rend.commandId          = SIF_CMD_OTHERDATA;

		SendPacket(&rend, sizeof(SIFRPCREQUESTEND));
	}
}

void CSIF::SendCallReply(uint32 serverId, const void* returnData)
{
	CLog::GetInstance().Print(LOG_NAME, "Processing call reply from serverId: 0x%08X\r\n", serverId);

	auto replyIterator(m_callReplies.find(serverId));
	assert(replyIterator != m_callReplies.end());
	if(replyIterator == m_callReplies.end()) return;

	auto& requestInfo(replyIterator->second);
	if(requestInfo.call.recv != 0 && returnData != nullptr)
	{
		uint32 dstPtr = requestInfo.call.recv & (PS2::EE_RAM_SIZE - 1);
		assert((dstPtr & 0x0F) == 0);
		//Size needs to be a multiple of 4
		assert((requestInfo.call.recvSize & 0x03) == 0);
		uint32 dstSize = (requestInfo.call.recvSize + 0x03) & ~0x03;
		memcpy(m_eeRam + dstPtr, returnData, dstSize);
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
		return 1;
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Warning. Trying to read an invalid system register (0x%08X).\r\n", nRegister);
		return 0;
		break;
	}
}

void CSIF::SetRegister(uint32 nRegister, uint32 nValue)
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
		CLog::GetInstance().Print(LOG_NAME, "Warning. Trying to write to an invalid system register (0x%08X).\r\n", nRegister);
		break;
	}
}
