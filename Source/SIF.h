#pragma once

#include <map>
#include <vector>
#include "SifDefs.h"
#include "SifModule.h"
#include "DMAC.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "RegisterStateFile.h"
#include "StructFile.h"

class CSIF
{
public:

									CSIF(CDMAC&, uint8*, uint8*);
	virtual							~CSIF();

	void							Reset();
	
	void							ProcessPackets();
	void							MarkPacketProcessed();

	void							RegisterModule(uint32, CSifModule*);
	bool							IsModuleRegistered(uint32) const;
	void							UnregisterModule(uint32);
	void							SetDmaBuffer(uint32, uint32);
	void							SendCallReply(uint32, const void*);

	uint32							ReceiveDMA5(uint32, uint32, uint32, bool);
	uint32							ReceiveDMA6(uint32, uint32, uint32, bool);

	void							SendPacket(void*, uint32);

	void							SendDMA(void*, uint32);

	uint32							GetRegister(uint32);
	void							SetRegister(uint32, uint32);

	void							LoadState(Framework::CZipArchiveReader&);
	void							SaveState(Framework::CZipArchiveWriter&);

private:
	enum CONST_MAX_USERREG
	{
		MAX_USERREG = 0x10,
	};

	struct RPCOTHERDATA
	{
		PACKETHDR					Header;
		uint32						nRecordID;
		uint32						nPacketAddr;
		uint32						nRPCID;
		uint32						nReceiveDataAddr;
		uint32						nSrcPtr;
		uint32						nDstPtr;
		uint32						nSize;
	};

	struct SETSREG
	{
		PACKETHDR					Header;
		uint32						nRegister;
		uint32						nValue;
	};

	struct CALLREQUESTINFO
	{
		RPCCALL						call;
		RPCREQUESTEND				reply;
	};

	typedef std::map<uint32, CSifModule*> ModuleMap;
	typedef std::vector<uint8> PacketQueue;
	typedef std::map<uint32, CALLREQUESTINFO> CallReplyMap;
	typedef std::map<uint32, RPCREQUESTEND> BindReplyMap;

	void							DeleteModules();

	void							SaveState_Header(const std::string&, CStructFile&, const PACKETHDR&);
	void							SaveState_RpcCall(CStructFile&, const RPCCALL&);
	void							SaveState_RequestEnd(CStructFile&, const RPCREQUESTEND&);

	void							LoadState_Header(const std::string&, const CStructFile&, PACKETHDR&);
	void							LoadState_RpcCall(const CStructFile&, RPCCALL&);
	void							LoadState_RequestEnd(const CStructFile&, RPCREQUESTEND&);

	void							Cmd_SetEERecvAddr(PACKETHDR*);
	void							Cmd_Initialize(PACKETHDR*);
	void							Cmd_Bind(PACKETHDR*);
	void							Cmd_Call(PACKETHDR*);
	void							Cmd_GetOtherData(PACKETHDR*);

	uint8*							m_eeRam;
	uint8*							m_iopRam;
	uint8*							m_dmaBuffer;
	uint32							m_dmaBufferSize;
	CDMAC&							m_dmac;

	uint32							m_nMAINADDR;
	uint32							m_nSUBADDR;
	uint32							m_nMSFLAG;
	uint32							m_nSMFLAG;

	uint32							m_nEERecvAddr;
	uint32							m_nDataAddr;

	uint32							m_nUserReg[MAX_USERREG];

	ModuleMap						m_modules;

	PacketQueue						m_packetQueue;
	bool							m_packetProcessed;

	CallReplyMap					m_callReplies;
	BindReplyMap					m_bindReplies;
};

#endif
