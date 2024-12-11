#include <cassert>
#include <cstring>
#include "OpticalMedia.h"

#define DVD_LAYER_MAX_BLOCKS 2295104

std::unique_ptr<COpticalMedia> COpticalMedia::CreateAuto(const StreamPtr& stream, uint32 createFlags)
{
	auto result = std::make_unique<COpticalMedia>();
	//Simulate a disk with only one data track
	try
	{
		auto blockProvider = std::make_shared<ISO9660::CBlockProvider2048>(stream);
		result->m_fileSystem = std::make_unique<CISO9660>(blockProvider);
		result->m_mediaBlockType = MEDIA_BLOCK_TYPE_2048;
		result->m_blockProvider = blockProvider;
	}
	catch(...)
	{
		//Failed with block size 2048, try with CD-ROM XA
		auto blockProvider = std::make_shared<ISO9660::CBlockProviderCDROMXA>(stream);
		result->m_fileSystem = std::make_unique<CISO9660>(blockProvider);
		result->m_mediaBlockType = MEDIA_BLOCK_TYPE_2352;
		result->m_blockProvider = blockProvider;
	}

	if(
	    (result->m_mediaBlockType == MEDIA_BLOCK_TYPE_2048) &&
	    !(createFlags & CREATE_AUTO_DISABLE_DL_DETECT))
	{
		try
		{
			result->CheckDualLayerDvd(stream);
			result->SetupSecondLayer(stream);
		}
		catch(...)
		{
			//Failed to check if we got a dual layer DVD (ex.: Couldn't get stream size of physical disc)
		}
	}
	
	if(!(createFlags & CREATE_AUTO_NO_FIRST_TRACK))
	{
		TRACK track = {};
		track.size = result->m_blockProvider->GetBlockCount();
		result->AddTrack(track);
	}
	
	return result;
}

std::unique_ptr<COpticalMedia> COpticalMedia::CreateDvd(const StreamPtr& stream, bool isDualLayer, uint32 secondLayerStart)
{
	auto result = std::make_unique<COpticalMedia>();
	auto blockProvider = std::make_shared<ISO9660::CBlockProvider2048>(stream);
	result->m_fileSystem = std::make_unique<CISO9660>(blockProvider);
	result->m_mediaBlockType = MEDIA_BLOCK_TYPE_2048;
	result->m_blockProvider = blockProvider;
	result->m_dvdIsDualLayer = isDualLayer;
	result->m_dvdSecondLayerStart = secondLayerStart;
	result->SetupSecondLayer(stream);
	return result;
}

std::unique_ptr<COpticalMedia> COpticalMedia::CreateCustom(BlockProviderPtr blockProvider, MEDIA_BLOCK_TYPE mediaBlockType, std::vector<TRACK> tracks)
{
	auto result = std::make_unique<COpticalMedia>();
	result->m_fileSystem = std::make_unique<CISO9660>(blockProvider);
	result->m_mediaBlockType = mediaBlockType;
	result->m_blockProvider = std::move(blockProvider);
	result->m_tracks = std::move(tracks);
	return result;
}

void COpticalMedia::AddTrack(const TRACK& track)
{
	m_tracks.push_back(track);
}

uint32 COpticalMedia::GetTrackCount() const
{
	return static_cast<uint32>(m_tracks.size());
}

const COpticalMedia::TRACK& COpticalMedia::GetTrack(uint32 trackIndex) const
{
	return m_tracks[trackIndex];
}

COpticalMedia::MEDIA_BLOCK_TYPE COpticalMedia::GetMediaBlockType() const
{
	return m_mediaBlockType;
}

ISO9660::CBlockProvider* COpticalMedia::GetBlockProvider() const
{
	return m_blockProvider.get();
}

CISO9660* COpticalMedia::GetFileSystem()
{
	return m_fileSystem.get();
}

CISO9660* COpticalMedia::GetFileSystemL1()
{
	return m_fileSystemL1.get();
}

bool COpticalMedia::GetDvdIsDualLayer() const
{
	return m_dvdIsDualLayer;
}

uint32 COpticalMedia::GetDvdSecondLayerStart() const
{
	//The PS2 seems to report a second layer LBN that is 0x10
	//sectors less than what the actual layer break position is
	return m_dvdSecondLayerStart - 0x10;
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

	//This is not by all means perfect, some numbers to take in consideration:
	//- Wild Arms: Alter Code F (US) second layer starts at around 49% of the disc's size.
	//- MGS2: Substance (US) second layer starts at around 37% of the disc's size.

	//Start looking at about 35% of the disc's size
	auto searchBlockAddress = imageBlockCount * 7 / 20;
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

void COpticalMedia::SetupSecondLayer(const StreamPtr& stream)
{
	if(!m_dvdIsDualLayer) return;
	auto blockProvider = std::make_shared<ISO9660::CBlockProvider2048>(stream, GetDvdSecondLayerStart());
	m_fileSystemL1 = std::make_unique<CISO9660>(blockProvider);
}
