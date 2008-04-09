#ifndef _MEMORYUTILS_H_
#define _MEMORYUTILS_H_

#include "MIPS.h"

class CMemoryUtils
{
public:
	//Proxies
	static uint32		GetByteProxy(CMIPS*, uint32);
	static uint32		GetHalfProxy(CMIPS*, uint32);
	static uint32		GetWordProxy(CMIPS*, uint32);
	static void			SetByteProxy(CMIPS*, uint32, uint32);
	static void			SetHalfProxy(CMIPS*, uint32, uint32);
	static void			SetWordProxy(CMIPS*, uint32, uint32);
};

#endif
