#pragma once

#include "Types.h"

enum CONST_SIF_CMD
{
	SIF_CMD_INIT		= 0x80000002,
	SIF_CMD_REND		= 0x80000008,
	SIF_CMD_BIND		= 0x80000009,
	SIF_CMD_CALL		= 0x8000000A,
	SIF_CMD_OTHERDATA	= 0x8000000C,
};

struct PACKETHDR
{
	uint32						nSize;
	uint32						nDest;
	uint32						nCID;
	uint32						nOptional;
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

struct SIFRPCHEADER
{
	uint32		packetAddr;
	uint32		rpcId;
	uint32		semaId;
	uint32		mode;
};
static_assert(sizeof(SIFRPCHEADER) == 0x10, "sizeof(SIFRPCHEADER) must be 16 bytes.");

struct SIFRPCCLIENTDATA
{
	SIFRPCHEADER	header;
	uint32			command;
	uint32			buffPtr;
	uint32			cbuffPtr;
	uint32			endFctPtr;
	uint32			endParam;
	uint32			serverDataAddr;
};
static_assert(sizeof(SIFRPCCLIENTDATA) == 0x28, "sizeof(SIFRPCCLIENTDATA) must be 40 bytes.");
