#pragma once

#include "Types.h"
#include "gs/GSHandler.h"
#include "Stream.h"
#include "Ps2Const.h"
#include "MIPS.h"

class CGsPacketMetadata
{
public:
#ifdef DEBUGGER_INCLUDED
	explicit CGsPacketMetadata(unsigned int pathIndex = 0)
		: pathIndex(pathIndex)
		, vuMemPacketAddress(0)
		, vpu1Top(0)
		, vpu1Itop(0)
	{
		memset(&vu1State, 0, sizeof(vu1State));
		memset(microMem1, 0, sizeof(microMem1));
		memset(vuMem1, 0, sizeof(vuMem1));
	}

	unsigned int	pathIndex;
	MIPSSTATE		vu1State;
	uint8			microMem1[PS2::MICROMEM1SIZE];
	uint8			vuMem1[PS2::VUMEM1SIZE];
	uint32			vpu1Top;
	uint32			vpu1Itop;
	uint32			vuMemPacketAddress;
#else
	explicit CGsPacketMetadata(unsigned int = 0)
	{

	}
#endif
};

class CGsPacket
{
public:
	typedef std::vector<CGSHandler::RegisterWrite> WriteArray;

	WriteArray			writes;
	CGsPacketMetadata	metadata;
};

struct DRAWINGKICK_INFO
{
	struct VERTEX
	{
		uint16 x = 0;
		uint16 y = 0;
	};

	unsigned int	context = 0;
	unsigned int	primType = CGSHandler::PRIM_INVALID;
	VERTEX			vertex[3];
};

typedef std::map<uint32, DRAWINGKICK_INFO> DrawingKickInfoMap;

class CFrameDump
{
public:
	typedef std::vector<CGsPacket> PacketArray;

								CFrameDump();
	virtual						~CFrameDump();

	void						Reset();

	uint8*						GetInitialGsRam();
	uint64*						GetInitialGsRegisters();

	uint64						GetInitialSMODE2() const;
	void						SetInitialSMODE2(uint64);

	const PacketArray&			GetPackets() const;
	void						AddPacket(const CGSHandler::RegisterWrite*, uint32, const CGsPacketMetadata*);

	void						Read(Framework::CStream&);
	void						Write(Framework::CStream&) const;

	void						IdentifyDrawingKicks();
	const DrawingKickInfoMap&	GetDrawingKicks() const;

private:
	uint8*						m_initialGsRam = nullptr;
	uint64						m_initialGsRegisters[CGSHandler::REGISTER_MAX];
	uint64						m_initialSMODE2 = 0;
	PacketArray					m_packets;
	DrawingKickInfoMap			m_drawingKicks;
};
