#pragma once

#include "OpticalMedia.h"
#include <future>

namespace DiscUtils
{
	typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;

	struct RangeChecksumTask
	{
		std::promise<uint32> value;
		std::atomic<float> progress = 0;
		std::atomic<bool> cancelFlag = false;
	};
	typedef std::shared_ptr<RangeChecksumTask> RangeChecksumTaskPtr;

	RangeChecksumTaskPtr RangeChecksumAsync(OpticalMediaPtr&, uint32, uint32);

	RangeChecksumTaskPtr ChecksumAsync(OpticalMediaPtr&);
	RangeChecksumTaskPtr TrackChecksumAsync(OpticalMediaPtr&, uint32);
}
