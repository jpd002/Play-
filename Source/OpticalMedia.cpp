#include <cassert>
#include <cstring>
#include "OpticalMedia.h"

#define DVD_LAYER_MAX_BLOCKS 2295104

COpticalMedia* COpticalMedia::CreateAuto(StreamPtr& stream)
{
	auto result = new COpticalMedia();
	//Simulate a disk with only one data track
	try
	{
		auto blockProvider = std::make_shared<ISO9660::CBlockProvider2048>(stream);
		result->m_fileSystem = std::make_unique<CISO9660>(blockProvider);
		result->m_track0DataType = TRACK_DATA_TYPE_MODE1_2048;
	}
	catch(...)
	{
		//Failed with block size 2048, try with CD-ROM XA
		auto blockProvider = std::make_shared<ISO9660::CBlockProviderCDROMXA>(stream);
		result->m_fileSystem = std::make_unique<CISO9660>(blockProvider);
		result->m_track0DataType = TRACK_DATA_TYPE_MODE2_2352;
	}

	if(result->m_track0DataType == TRACK_DATA_TYPE_MODE1_2048)
	{
		result->CheckDualLayerDvd(stream);
	}
	return result;
}

COpticalMedia* COpticalMedia::CreateDvd(StreamPtr& stream, bool isDualLayer, uint32 secondLayerStart)
{
	auto result = new COpticalMedia();
	result->m_dvdIsDualLayer = isDualLayer;
	result->m_dvdSecondLayerStart = secondLayerStart;
	return result;
}

COpticalMedia::TRACK_DATA_TYPE COpticalMedia::GetTrackDataType(uint32 trackIndex) const
{
	assert(trackIndex == 0);
	return m_track0DataType;
}

CISO9660* COpticalMedia::GetFileSystem()
{
	return m_fileSystem.get();
}

bool COpticalMedia::GetDvdIsDualLayer() const
{
	return m_dvdIsDualLayer;
}

uint32 COpticalMedia::GetDvdSecondLayerStart() const
{
	return m_dvdSecondLayerStart;
}

void COpticalMedia::CheckDualLayerDvd(const StreamPtr& stream)
{
	//Heuristic to detect dual layer DVD disc images

	static const uint32 blockSize = 2048;
	auto imageSize = stream->GetLength();
	uint32 imageBlockCount = static_cast<uint32>(imageSize / blockSize);

	//DL discs may be smaller than the capacity of a SL DVD, but we assume
	//that games that use DL discs use more than the SL DVD capacity
	if(imageBlockCount < DVD_LAYER_MAX_BLOCKS) return;

	//Bigger than the capacity of a SL DVD, certainly a dual layer disc
	m_dvdIsDualLayer = true;

	//We need to look for the start of the second layer
	//The second layer is at most as big as the first one, so
	//we can start looking from half of the disk image

	//NOTE: Wild Arms: Alter Code F seems to have a second layer that's a bit
	//larger than the first one? That's why we start looking at 15 / 32 of the image's size

	auto searchBlockAddress = imageBlockCount * 15 / 32;
	stream->Seek(static_cast<uint64>(searchBlockAddress) * blockSize, Framework::STREAM_SEEK_SET);

	//Scan all blocks from the search point, looking for a valid ISO9660 descriptor
	for(auto lba = searchBlockAddress; lba < imageBlockCount; lba++)
	{
		static const uint32 blockHeaderSize = 6;
		static_assert(blockSize >= blockHeaderSize, "Block header size is too large");
		char blockHeader[blockHeaderSize];
		stream->Read(blockHeader, blockHeaderSize);
		if(
		    (blockHeader[0] == 0x01) &&
		    (!strncmp(blockHeader + 1, "CD001", 5)))
		{
			//We've found a valid ISO9660 descriptor
			m_dvdSecondLayerStart = lba;
			break;
		}
		stream->Seek(blockSize - blockHeaderSize, Framework::STREAM_SEEK_CUR);
	}

	//If we haven't found it, something's wrong
	assert(m_dvdSecondLayerStart != 0);
}
