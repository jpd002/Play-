#pragma once

#include "Types.h"
#include "INTC.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CTimer
{
public:
	enum
	{
		MODE_GATE_ENABLE        = 0x004,
		
		MODE_GATE_SELECT        = 0x008,
		MODE_GATE_SELECT_HBLANK = 0x000,
		MODE_GATE_SELECT_VBLANK = 0x008,

		MODE_GATE_MODE          = 0x030,
		MODE_GATE_MODE_COUNTLOW = 0x000,
		MODE_GATE_MODE_HIGHEDGE = 0x010,
		MODE_GATE_MODE_LOWEDGE  = 0x020,
		MODE_GATE_MODE_BOTHEDGE = 0x030,
		
		MODE_ZERO_RETURN        = 0x040,
		MODE_COUNT_ENABLE       = 0x080,
		MODE_EQUAL_FLAG         = 0x400,
		MODE_OVERFLOW_FLAG      = 0x800,
	};

							CTimer(CINTC&);
	virtual					~CTimer() = default;

	void					Reset();

	void					Count(unsigned int);

	uint32					GetRegister(uint32);
	void					SetRegister(uint32, uint32);

	void					LoadState(Framework::CZipArchiveReader&);
	void					SaveState(Framework::CZipArchiveWriter&);

	void					NotifyVBlankStart();
	void					NotifyVBlankEnd();

private:
	enum
	{
		MAX_TIMER = 4,
	};

	void					DisassembleGet(uint32);
	void					DisassembleSet(uint32, uint32);

	void					ProcessGateEdgeChange(uint32, uint32);

	struct TIMER
	{
		uint32	nCOUNT;
		uint32	nMODE;
		uint32	nCOMP;
		uint32	nHOLD;

		uint32	clockRemain;
	};

	TIMER					m_timer[MAX_TIMER];
	CINTC&					m_intc;
};
