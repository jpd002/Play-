#pragma once

#include "Types.h"
#include "GSHandler.h"
#include "Stream.h"

class CFrameDump
{
public:
	typedef std::vector<CGSHandler::RegisterWrite> Packet;
	typedef std::vector<Packet> PacketArray;

							CFrameDump();
	virtual					~CFrameDump();

	void					Reset();

	uint8*					GetInitialGsRam();
	uint64*					GetInitialGsRegisters();

	const PacketArray&		GetPackets() const;
	void					AddPacket(const CGSHandler::RegisterWrite*, uint32);

	void					Read(Framework::CStream&);
	void					Write(Framework::CStream&) const;

private:
	uint8*					m_initialGsRam;
	uint64					m_initialGsRegisters[CGSHandler::REGISTER_MAX];
	PacketArray				m_packets;
};
