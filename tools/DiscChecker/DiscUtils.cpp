#include "DiscUtils.h"
#include <cassert>
#include <zlib.h>
#include "DiskUtils.h"
#include "ISO9660/BlockProvider.h"

using namespace DiscUtils;

uint32 DiscUtils::RangeChecksum(OpticalMediaPtr& opticalMedia, uint32 start, uint32 size)
{
	auto blockProvider = opticalMedia->GetBlockProvider();
	const uint32 blockSize = blockProvider->GetMediaBlockSize();
	auto buffer = new uint8[blockSize];
	uint32 checksum = 0;
	for(uint32 i = 0; i < size; i++)
	{
		blockProvider->ReadMediaBlock(i + start, buffer);
		checksum = crc32(checksum, reinterpret_cast<Bytef*>(buffer), blockSize);
	}
	delete[] buffer;
	return checksum;
}

uint32 DiscUtils::Checksum(OpticalMediaPtr& opticalMedia)
{
	auto blockProvider = opticalMedia->GetBlockProvider();
	uint32 blockCount = blockProvider->GetBlockCount();
	return RangeChecksum(opticalMedia, 0, blockCount);
}

uint32 DiscUtils::TrackChecksum(OpticalMediaPtr& opticalMedia, uint32 trackIndex)
{
	auto track = opticalMedia->GetTrack(trackIndex);
	return RangeChecksum(opticalMedia, track.start, track.size);
}
