#include "HighResTimer.h"
#ifdef WIN32
#include <windows.h>
#endif

#define NANOSEC (1000 * 1000 * 1000)

#ifdef WIN32
static LARGE_INTEGER frequency;
static BOOL initialized = QueryPerformanceFrequency(&frequency);
#endif

uint64 CHighResTimer::GetTime()
{
#ifdef WIN32
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return count.QuadPart;
#else
	return gethrtime();
#endif
}

uint64 CHighResTimer::GetDiff(uint64 prevTime, TIMESCALE scale)
{
#ifdef WIN32
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return ((count.QuadPart - prevTime) * static_cast<uint64>(scale)) / frequency.QuadPart;
#else
	return gethrtime() - prevTime;
#endif
}
