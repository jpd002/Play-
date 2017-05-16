#include "OpticalMedia.h"

COpticalMedia::COpticalMedia(const StreamPtr& stream)
{
	//Simulate a disk with only one data track
	try
	{
		auto blockProvider = std::make_shared<ISO9660::CBlockProvider2048>(stream);
		m_fileSystem = std::make_unique<CISO9660>(blockProvider);
		m_track0DataType = TRACK_DATA_TYPE_MODE1_2048;
	}
	catch(...)
	{
		//Failed with block size 2048, try with CD-ROM XA
		auto blockProvider = std::make_shared<ISO9660::CBlockProviderCDROMXA>(stream);
		m_fileSystem = std::make_unique<CISO9660>(blockProvider);
		m_track0DataType = TRACK_DATA_TYPE_MODE2_2352;
	}
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
