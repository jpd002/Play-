#pragma once

#include "OpticalMedia.h"

namespace DiscUtils
{
	typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;

	uint32 RangeChecksum(OpticalMediaPtr&, uint32, uint32);

	uint32 Checksum(OpticalMediaPtr&);
	uint32 TrackChecksum(OpticalMediaPtr&, uint32);
}
