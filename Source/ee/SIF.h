#pragma once

#include <map>
#include <vector>
#include "../SifDefs.h"
#include "../SifModule.h"
#include "DMAC.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "../RegisterStateFile.h"
#include "../StructFile.h"

class CSIF
{
public:
	typedef std::function<void(const std::string&)> ModuleResetHandler;
	typedef std::function<void(uint32)> CustomCommandHandler;

	CSIF(CDMAC&, uint8*, uint8*);
	virtual ~CSIF() = default;

	void Reset();

	void ProcessPackets();
	void MarkPacketProcessed();

	void RegisterModule(uint32, CSifModule*);
	bool IsModuleRegistered(uint32) const;
	void UnregisterModule(uint32);
	void SetDmaBuffer(uint32, uint32);
	void SetCmdBuffer(uint32, uint32);
	void SendCallReply(uint32, const void*);
	void SetModuleResetHandler(const ModuleResetHandler&);
	void SetCustomCommandHandler(const CustomCommandHandler&);

	uint32 ReceiveDMA5(uint32, uint32, uint32, bool);
	uint32 ReceiveDMA6(uint32, uint32, uint32, bool);

	void SendPacket(void*, uint32);

	void SendDMA(void*, uint32);

	uint32 GetRegister(uint32);
	void SetRegister(uint32, uint32);

	void LoadState(Framework::CZipArchiveReader&);
	void SaveState(Framework::CZipArchiveWriter&);

private:
	struct CALLREQUESTINFO
	{
		SIFRPCCALL call;
		SIFRPCREQUESTEND reply;
	};

	typedef std::map<uint32, CSifModule*> ModuleMap;
	typedef std::vector<uint8> PacketQueue;
	typedef std::map<uint32, CALLREQUESTINFO> CallReplyMap;
	typedef std::map<uint32, SIFRPCREQUESTEND> BindReplyMap;

	void DeleteModules();

	void SaveCallReplies(Framework::CZipArchiveWriter&);
	void SaveBindReplies(Framework::CZipArchiveWriter&);

	static PacketQueue LoadPacketQueue(Framework::CZipArchiveReader&);
	static CallReplyMap LoadCallReplies(Framework::CZipArchiveReader&);
	static BindReplyMap LoadBindReplies(Framework::CZipArchiveReader&);

	static void SaveState_Header(const std::string&, CStructFile&, const SIFCMDHEADER&);
	static void SaveState_RpcCall(CStructFile&, const SIFRPCCALL&);
	static void SaveState_RequestEnd(CStructFile&, const SIFRPCREQUESTEND&);

	static void LoadState_Header(const std::string&, const CStructFile&, SIFCMDHEADER&);
	static void LoadState_RpcCall(const CStructFile&, SIFRPCCALL&);
	static void LoadState_RequestEnd(const CStructFile&, SIFRPCREQUESTEND&);

	void Cmd_SetEERecvAddr(const SIFCMDHEADER*);
	void Cmd_Initialize(const SIFCMDHEADER*);
	void Cmd_Bind(const SIFCMDHEADER*);
	void Cmd_Call(const SIFCMDHEADER*);
	void Cmd_GetOtherData(const SIFCMDHEADER*);

	uint8* m_eeRam;
	uint8* m_iopRam;
	uint32 m_dmaBufferAddress = 0;
	uint32 m_dmaBufferSize = 0;
	uint32 m_cmdBufferAddress = 0;
	uint32 m_cmdBufferSize = 0;
	CDMAC& m_dmac;

	uint32 m_nMAINADDR = 0;
	uint32 m_nSUBADDR = 0;
	uint32 m_nMSFLAG = 0;
	uint32 m_nSMFLAG = 0;

	uint32 m_nEERecvAddr = 0;
	uint32 m_nDataAddr = 0;

	ModuleMap m_modules;

	PacketQueue m_packetQueue;
	bool m_packetProcessed;

	CallReplyMap m_callReplies;
	BindReplyMap m_bindReplies;

	ModuleResetHandler m_moduleResetHandler;
	CustomCommandHandler m_customCommandHandler;
};
