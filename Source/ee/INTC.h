#pragma once

#include "Types.h"
#include "DMAC.h"
#include "../gs/GSHandler.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CINTC
{
public:
	enum REGISTER
	{
		INTC_STAT = 0x1000F000,
		INTC_MASK = 0x1000F010,
	};

	enum LINES
	{
		INTC_LINE_GS			= 0,
		INTC_LINE_DMAC			= 1,
		INTC_LINE_VBLANK_START	= 2,
		INTC_LINE_VBLANK_END	= 3,
		INTC_LINE_VIF0			= 4,
		INTC_LINE_VIF1			= 5,
		INTC_LINE_TIMER0		= 9,
		INTC_LINE_TIMER1		= 10,
		INTC_LINE_TIMER2		= 11,
		INTC_LINE_TIMER3		= 12,
	};

					CINTC(CDMAC&, CGSHandler*&);
	virtual			~CINTC();

	void			Reset();
	bool			IsInterruptPending();

	uint32			GetRegister(uint32);
	void			SetRegister(uint32, uint32);

	void			AssertLine(uint32);

	void			LoadState(Framework::CZipArchiveReader&);
	void			SaveState(Framework::CZipArchiveWriter&);

private:
	uint32			m_INTC_STAT;
	uint32			m_INTC_MASK;
	CDMAC&			m_dmac;
	CGSHandler*&	m_gs;
};
