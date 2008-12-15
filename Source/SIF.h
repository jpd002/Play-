#ifndef _SIF_H_
#define _SIF_H_

#include <map>
#include <vector>
#include "SifModule.h"
#include "DMAC.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CSIF
{
public:
	struct PACKETHDR
	{
		uint32						nSize;
		uint32						nDest;
		uint32						nCID;
		uint32						nOptional;
	};

                                    CSIF(CDMAC&, uint8*, uint8*);
    virtual                         ~CSIF();

	void							Reset();
	
    void                            ProcessPackets();

    void                            RegisterModule(uint32, CSifModule*);
    void                            SetDmaBuffer(uint8*, uint32);
    void                            SendCallReply(uint32, void*);

    uint32                          ReceiveDMA5(uint32, uint32, uint32, bool);
	uint32							ReceiveDMA6(uint32, uint32, uint32, bool);

    void                            SendPacket(void*, uint32);

	void							SendDMA(void*, uint32);

	uint32							GetRegister(uint32);
	void							SetRegister(uint32, uint32);

	void							LoadState(CZipArchiveReader&);
	void							SaveState(CZipArchiveWriter&);

private:
	enum CONST_MAX_USERREG
	{
		MAX_USERREG = 0x10,
	};

	enum CONST_SIF_CMD
	{
		SIF_CMD_INIT = 0x80000002,
		SIF_CMD_REND = 0x80000008,
		SIF_CMD_BIND = 0x80000009,
		SIF_CMD_CALL = 0x8000000A,
	};

	struct RPCREQUESTEND
	{
		PACKETHDR					Header;
		uint32						nRecordID;
		uint32						nPacketAddr;
		uint32						nRPCID;
		uint32						nClientDataAddr;
		uint32						nCID;
		uint32						nServerDataAddr;
		uint32						nBuffer;
		uint32						nClientBuffer;
	};

	struct RPCBIND
	{
		PACKETHDR					Header;
		uint32						nRecordID;
		uint32						nPacketAddr;
		uint32						nRPCID;
		uint32						nClientDataAddr;
		uint32						nSID;
	};

	struct RPCCALL
	{
		PACKETHDR					Header;
		uint32						nRecordID;
		uint32						nPacketAddr;
		uint32						nRPCID;
		uint32						nClientDataAddr;
		uint32						nRPCNumber;
		uint32						nSendSize;
		uint32						nRecv;
		uint32						nRecvSize;
		uint32						nRecvMode;
		uint32						nServerDataAddr;
	};

	struct SETSREG
	{
		PACKETHDR					Header;
		uint32						nRegister;
		uint32						nValue;
	};

    struct CALLREQUESTINFO
    {
        RPCCALL                     call;
        RPCREQUESTEND               reply;
    };

    typedef std::map<uint32, CSifModule*> ModuleMap;
    typedef std::vector<uint8> PacketQueue;
    typedef std::map<uint32, CALLREQUESTINFO> CallReplyMap;
    typedef std::map<uint32, RPCREQUESTEND> BindReplyMap;

    void							DeleteModules();

	void							Cmd_SetEERecvAddr(PACKETHDR*);
	void							Cmd_Initialize(PACKETHDR*);
	void							Cmd_Bind(PACKETHDR*);
	void							Cmd_Call(PACKETHDR*);

    uint8*                          m_eeRam;
    uint8*                          m_iopRam;
    uint8*                          m_dmaBuffer;
    uint32                          m_dmaBufferSize;
    CDMAC&                          m_dmac;

	uint32							m_nMAINADDR;
	uint32							m_nSUBADDR;
	uint32							m_nMSFLAG;
	uint32							m_nSMFLAG;

	uint32							m_nEERecvAddr;
	uint32							m_nDataAddr;

	uint32							m_nUserReg[MAX_USERREG];

	ModuleMap	                    m_modules;
    PacketQueue                     m_packetQueue;
    CallReplyMap                    m_callReplies;
    BindReplyMap                    m_bindReplies;
};

#endif
