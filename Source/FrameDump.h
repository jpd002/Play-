#pragma once

#include "Types.h"
#include "GSHandler.h"
#include "Stream.h"
#include "Ps2Const.h"
#include "MIPS.h"

class CGsPacketMetadata
{
public:
	explicit CGsPacketMetadata(unsigned int pathIndex = 0)
		: pathIndex(pathIndex)
		, vuMemPacketAddress(0)
		, vpu1Top(0)
		, vpu1Itop(0)
	{

	}

	unsigned int	pathIndex;
	MIPSSTATE		vu1State;
	uint8			microMem1[PS2::MICROMEM1SIZE];
	uint8			vuMem1[PS2::VUMEM1SIZE];
	uint32			vpu1Top;
	uint32			vpu1Itop;
	uint32			vuMemPacketAddress;
};

class CGsPacket
{
public:
	typedef std::vector<CGSHandler::RegisterWrite> WriteArray;

	WriteArray			writes;
	CGsPacketMetadata	metadata;
};

class CFrameDump
{
public:
	typedef std::vector<CGsPacket> PacketArray;

							CFrameDump();
	virtual					~CFrameDump();

	void					Reset();

	uint8*					GetInitialGsRam();
	uint64*					GetInitialGsRegisters();

	const PacketArray&		GetPackets() const;
	void					AddPacket(const CGSHandler::RegisterWrite*, uint32, const CGsPacketMetadata*);

	void					Read(Framework::CStream&);
	void					Write(Framework::CStream&) const;

private:
	uint8*					m_initialGsRam;
	uint64					m_initialGsRegisters[CGSHandler::REGISTER_MAX];
	PacketArray				m_packets;
};
