#pragma once

#include "MIPS.h"

extern "C"
{
	uint32 MemoryUtils_GetByteProxy(CMIPS*, uint32);
	uint32 MemoryUtils_GetHalfProxy(CMIPS*, uint32);
	uint32 MemoryUtils_GetWordProxy(CMIPS*, uint32);
	uint64 MemoryUtils_GetDoubleProxy(CMIPS*, uint32);
	uint128 MemoryUtils_GetQuadProxy(CMIPS*, uint32);

	void MemoryUtils_SetByteProxy(CMIPS*, uint32, uint32);
	void MemoryUtils_SetHalfProxy(CMIPS*, uint32, uint32);
	void MemoryUtils_SetWordProxy(CMIPS*, uint32, uint32);
	void MemoryUtils_SetDoubleProxy(CMIPS*, uint64, uint32);
	void MemoryUtils_SetQuadProxy(CMIPS*, const uint128&, uint32);
}
