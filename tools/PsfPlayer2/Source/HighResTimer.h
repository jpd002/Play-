#ifndef _HIGHRESTIMER_H_
#define _HIGHRESTIMER_H_

#include "Types.h"

class CHighResTimer
{
public:
	enum TIMESCALE
	{
		MILLISECOND = (1000),
		MICROSECOND = (1000 * 1000),
		NANOSECOND = (1000 * 1000 * 1000)
	};

	static uint64 GetTime();
	static uint64 GetDiff(uint64, TIMESCALE);

private:
};

#endif
