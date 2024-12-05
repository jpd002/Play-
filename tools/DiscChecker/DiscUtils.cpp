#include "DiscUtils.h"
#include <cassert>
#include <zlib.h>
#include "DiskUtils.h"
#include "ISO9660/BlockProvider.h"

using namespace DiscUtils;

uint32 DiscUtils::Checksum(OpticalMediaPtr& opticalMedia)
{
	auto blockProvider = opticalMedia->GetBlockProvider();
	const uint32 blockSize = blockProvider->GetMediaBlockSize();
	uint32 blockCount = blockProvider->GetBlockCount();
	auto buffer = new uint8[blockSize];
	uint32 checksum = 0;
	for(uint32 i = 0; i < blockCount; i++)
	{
		blockProvider->ReadMediaBlock(i, buffer);
		checksum = crc32(checksum, reinterpret_cast<Bytef*>(buffer), blockSize);
	}
	delete[] buffer;
	return checksum;
}
