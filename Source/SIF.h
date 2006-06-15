#ifndef _SIF_H_
#define _SIF_H_

#include "List.h"
#include "Stream.h"
#include "IOP_Module.h"
#include "IOP_PadMan.h"
#include "IOP_DbcMan.h"
#include "IOP_FileIO.h"

class CSIF
{
public:
	static void								Reset();
	
	static uint32							ReceiveDMA(uint32, uint32, uint32);
	static void								SendDMA(void*, uint32);

	static uint32							GetRegister(uint32);
	static void								SetRegister(uint32, uint32);

	static void								LoadState(Framework::CStream*);
	static void								SaveState(Framework::CStream*);

	static IOP::CPadMan*					GetPadMan();
	static IOP::CDbcMan*					GetDbcMan();
	static IOP::CFileIO*					GetFileIO();

	//This will eventually be moved in the IOP
	static uint8							m_pRAM[0x1000];

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

	struct PACKETHDR
	{
		uint32								nSize;
		uint32								nDest;
		uint32								nCID;
		uint32								nOptional;
	};

	struct RPCREQUESTEND
	{
		PACKETHDR							Header;
		uint32								nRecordID;
		uint32								nPacketAddr;
		uint32								nRPCID;
		uint32								nClientDataAddr;
		uint32								nCID;
		uint32								nServerDataAddr;
		uint32								nBuffer;
		uint32								nClientBuffer;
	};

	struct RPCBIND
	{
		PACKETHDR							Header;
		uint32								nRecordID;
		uint32								nPacketAddr;
		uint32								nRPCID;
		uint32								nClientDataAddr;
		uint32								nSID;
	};

	struct RPCCALL
	{
		PACKETHDR							Header;
		uint32								nRecordID;
		uint32								nPacketAddr;
		uint32								nRPCID;
		uint32								nClientDataAddr;
		uint32								nRPCNumber;
		uint32								nSendSize;
		uint32								nRecv;
		uint32								nRecvSize;
		uint32								nRecvMode;
		uint32								nServerDataAddr;
	};

	struct SETSREG
	{
		PACKETHDR							Header;
		uint32								nRegister;
		uint32								nValue;
	};

	static void								DeleteModules();

	static void								Cmd_SetEERecvAddr(PACKETHDR*);
	static void								Cmd_Initialize(PACKETHDR*);
	static void								Cmd_Bind(PACKETHDR*);
	static void								Cmd_Call(PACKETHDR*);

	static uint32							m_nMAINADDR;
	static uint32							m_nSUBADDR;
	static uint32							m_nMSFLAG;
	static uint32							m_nSMFLAG;

	static uint32							m_nEERecvAddr;
	static uint32							m_nDataAddr;

	static uint32							m_nUserReg[MAX_USERREG];

	static IOP::CPadMan*					m_pPadMan;
	static Framework::CList<IOP::CModule>	m_Module;
};

#endif
