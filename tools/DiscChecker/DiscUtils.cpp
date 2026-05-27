#include "DiscUtils.h"
#include <cassert>
#include <zlib.h>
#include "DiskUtils.h"
#include "ISO9660/BlockProvider.h"

using namespace DiscUtils;

DiscUtils::RangeChecksumTaskPtr DiscUtils::RangeChecksumAsync(OpticalMediaPtr& opticalMedia, uint32 start, uint32 size)
{
	auto task = std::make_shared<RangeChecksumTask>();
	std::thread execThread(
	    [task, opticalMedia = opticalMedia.get(), start, size] {
		    auto blockProvider = opticalMedia->GetBlockProvider();
		    const uint32 blockSize = blockProvider->GetMediaBlockSize();
		    auto buffer = new uint8[blockSize];
		    uint32 checksum = 0;
		    for(uint32 i = 0; i < size; i++)
		    {
			    if(task->cancelFlag)
			    {
				    break;
			    }
			    task->progress = static_cast<float>(i) / static_cast<float>(size);
			    blockProvider->ReadMediaBlock(i + start, buffer);
			    checksum = crc32(checksum, reinterpret_cast<Bytef*>(buffer), blockSize);
		    }
		    delete[] buffer;
		    task->progress = 1.f;
		    task->value.set_value(checksum);
	    });
	execThread.detach();
	return task;
}

DiscUtils::RangeChecksumTaskPtr DiscUtils::ChecksumAsync(OpticalMediaPtr& opticalMedia)
{
	auto blockProvider = opticalMedia->GetBlockProvider();
	uint32 blockCount = blockProvider->GetBlockCount();
	return RangeChecksumAsync(opticalMedia, 0, blockCount);
}

DiscUtils::RangeChecksumTaskPtr DiscUtils::TrackChecksumAsync(OpticalMediaPtr& opticalMedia, uint32 trackIndex)
{
	auto track = opticalMedia->GetTrack(trackIndex);
	return RangeChecksumAsync(opticalMedia, track.start, track.size);
}
