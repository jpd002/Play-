#pragma once

#include "Types.h"
#include "../gs/GSHandler.h"
#include "../Profiler.h"

class CGIF
{
public:
	enum REGISTER
	{
		GIF_STAT	= 0x10003020
	};

	enum
	{
		GIF_STAT_APATH3 = 0xC00,
	};

	enum
	{
		REGS_START	= 0x10003000,
		REGS_END	= 0x100030B0,
	};

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
	uint32			ProcessPacket(uint8*, uint32, uint32, const CGsPacketMetadata&);

	uint32			GetRegister(uint32);
	void			SetRegister(uint32, uint32);

private:
	uint32			ProcessPacked(CGSHandler::RegisterWriteList&, uint8*, uint32, uint32);
	uint32			ProcessRegList(CGSHandler::RegisterWriteList&, uint8*, uint32, uint32);
	uint32			ProcessImage(uint8*, uint32, uint32);

	void			DisassembleGet(uint32);
	void			DisassembleSet(uint32, uint32);

	uint16			m_loops;
	uint8			m_cmd;
	uint8			m_regs;
	uint8			m_regsTemp;
	uint64			m_regList;
	bool			m_eop;
	uint32			m_qtemp;
	uint8*			m_ram;
	uint8*			m_spr;
	CGSHandler*&	m_gs;
	std::mutex		m_pathMutex;

	CProfiler::ZoneHandle m_gifProfilerZone = 0;
};
