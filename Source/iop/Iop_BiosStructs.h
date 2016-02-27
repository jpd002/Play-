#pragma once

#include "Types.h"

namespace Iop
{
	struct MEMORYBLOCK
	{
		uint32	isValid;
		uint32	nextBlock;
		uint32	address;
		uint32	size;
	};
};
