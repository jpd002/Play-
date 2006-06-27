#ifndef _INTC_H_
#define _INTC_H_

#include "Types.h"
#include "Stream.h"

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
		INTC_LINE_DMAC			= 1,
		INTC_LINE_VBLANK_START	= 2,
		INTC_LINE_VBLANK_END	= 3,
		INTC_LINE_TIMER2		= 11,
	};

	static void		Reset();
	static void		CheckInterrupts();

	static uint32	GetRegister(uint32);
	static void		SetRegister(uint32, uint32);

	static void		AssertLine(uint32);

	static void		LoadState(Framework::CStream*);
	static void		SaveState(Framework::CStream*);

private:
	static uint32	m_INTC_STAT;
	static uint32	m_INTC_MASK;
};

#endif
