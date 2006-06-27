#ifndef _TIMER_H_
#define _TIMER_H_

#include "Types.h"

class CTimer
{
public:
	enum REGISTERS
	{
		T0_COUNT	= 0x10000000,
		T0_MODE		= 0x10000010,
		T0_COMP		= 0x10000020,
		T0_HOLD		= 0x10000030,
	};

	static void					Reset();

	static void					Count(unsigned int);

	static uint32				GetRegister(uint32);
	static void					SetRegister(uint32, uint32);


private:
	static void					DisassembleGet(uint32);
	static void					DisassembleSet(uint32, uint32);

	struct TIMER
	{
		uint32	nCOUNT;
		uint32	nMODE;
		uint32	nCOMP;
		uint32	nHOLD;
	};

	static TIMER				m_Timer[4];
};

#endif
