#ifndef _TIMER_H_
#define _TIMER_H_

#include "Types.h"
#include "INTC.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CTimer
{
public:
	enum
	{
		MODE_ZERO_RETURN	= 0x040,
		MODE_COUNT_ENABLE	= 0x080,
		MODE_EQUAL_FLAG		= 0x400,
		MODE_OVERFLOW_FLAG	= 0x800,
	};

							CTimer(CINTC&);
	virtual					~CTimer();

	void					Reset();

	void					Count(unsigned int);

	uint32					GetRegister(uint32);
	void					SetRegister(uint32, uint32);

	void					LoadState(Framework::CZipArchiveReader&);
	void					SaveState(Framework::CZipArchiveWriter&);

private:
	void					DisassembleGet(uint32);
	void					DisassembleSet(uint32, uint32);

	struct TIMER
	{
		uint32	nCOUNT;
		uint32	nMODE;
		uint32	nCOMP;
		uint32	nHOLD;

		uint32	clockRemain;
	};

	TIMER					m_timer[4];
	CINTC&					m_intc;
};

#endif
