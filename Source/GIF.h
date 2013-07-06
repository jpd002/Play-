#pragma once

#include "Types.h"
#include "GSHandler.h"

class CGIF
{
public:
	struct TAG
	{
		unsigned int	loops		: 15;
		unsigned int	eop			: 1;
		unsigned int	reserved0	: 16;
		unsigned int	reserved1	: 14;
		unsigned int	pre			: 1;
		unsigned int	prim		: 11;
		unsigned int	cmd			: 2;
		unsigned int	nreg		: 4;
		uint64			regs;
	};
	static_assert(sizeof(TAG) == 0x10, "Size of TAG must be 16 bytes.");

					CGIF(CGSHandler*&, uint8*, uint8*);
	virtual			~CGIF();

	void			Reset();
	uint32			ReceiveDMA(uint32, uint32, uint32, bool);
	uint32			ProcessPacket(uint8*, uint32, uint32);

private:
	uint32			ProcessPacked(CGSHandler::RegisterWriteList&, uint8*, uint32, uint32);
	uint32			ProcessRegList(CGSHandler::RegisterWriteList&, uint8*, uint32, uint32);
	uint32			ProcessImage(uint8*, uint32, uint32);

	uint16			m_nLoops;
	uint8			m_nCmd;
	uint8			m_nRegs;
	uint8			m_nRegsTemp;
	uint64			m_nRegList;
	bool			m_nEOP;
	uint32			m_nQTemp;
	uint8*			m_ram;
	uint8*			m_spr;
	CGSHandler*&	m_gs;
	std::mutex		m_pathMutex;
};
