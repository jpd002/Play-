#pragma once

#include "OpticalMedia.h"

namespace DiscUtils
{
	typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;

	uint32 Checksum(OpticalMediaPtr&);
	uint32 PartialChecksum(OpticalMediaPtr&, uint64, uint64);
}
