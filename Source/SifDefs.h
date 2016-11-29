#pragma once

#include "Types.h"

enum CONST_SIF_CMD
{
	SIF_CMD_SETSREG      = 0x80000001,
	SIF_CMD_INIT         = 0x80000002,
	SIF_CMD_REND         = 0x80000008,
	SIF_CMD_BIND         = 0x80000009,
	SIF_CMD_CALL         = 0x8000000A,
	SIF_CMD_OTHERDATA    = 0x8000000C,
};

struct SIFDMAREG
{
	uint32 srcAddr;
	uint32 dstAddr;
	uint32 size;
	uint32 flags;
};
static_assert(sizeof(SIFDMAREG) == 0x10, "sizeof(SIFDMAREG) must be 16 bytes.");

struct SIFCMDHEADER
{
	uint32 packetSize : 8;
	uint32 destSize   : 24;
	uint32 dest;
	uint32 commandId;
	uint32 optional;
};
static_assert(sizeof(SIFCMDHEADER) == 0x10, "sizeof(SIFCMDHEADER) must be 16 bytes.");

struct SIFSETSREG
{
	SIFCMDHEADER    header;
	uint32          index;
	uint32          value;
};
static_assert(sizeof(SIFSETSREG) == 0x18, "sizeof(SIFSETREG) must be 24 bytes.");

struct SIFRPCBIND
{
	SIFCMDHEADER	header;
	uint32			recordId;
	uint32			packetAddr;
	uint32			rpcId;
	uint32			clientDataAddr;
	uint32			serverId;
};
static_assert(sizeof(SIFRPCBIND) == 0x24, "sizeof(SIFRPCBIND) must be 36 bytes.");

struct SIFRPCCALL
{
	SIFCMDHEADER	header;
	uint32			recordId;
	uint32			packetAddr;
	uint32			rpcId;
	uint32			clientDataAddr;
	uint32			rpcNumber;
	uint32			sendSize;
	uint32			recv;
	uint32			recvSize;
	uint32			recvMode;
	uint32			serverDataAddr;
};
static_assert(sizeof(SIFRPCCALL) == 0x38, "sizeof(SIFRPCCALL) must be 56 bytes.");

struct SIFRPCREQUESTEND
{
	SIFCMDHEADER	header;
	uint32			recordId;
	uint32			packetAddr;
	uint32			rpcId;
	uint32			clientDataAddr;
	uint32			commandId;
	uint32			serverDataAddr;
	uint32			buffer;
	uint32			cbuffer;
};
static_assert(sizeof(SIFRPCREQUESTEND) == 0x30, "sizeof(SIFRPCREQUESTEND) must be 48 bytes.");

struct SIFRPCOTHERDATA
{
	SIFCMDHEADER	header;
	uint32			recordId;
	uint32			packetAddr;
	uint32			rpcId;
	uint32			receiveDataAddr;
	uint32			srcPtr;
	uint32			dstPtr;
	uint32			size;
};
static_assert(sizeof(SIFRPCOTHERDATA) == 0x2C, "sizeof(SIFRPCOTHERDATA) must be 44 bytes.");

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
