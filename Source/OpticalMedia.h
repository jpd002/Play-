#pragma once

#include "Stream.h"
#include "ISO9660/ISO9660.h"

class COpticalMedia
{
public:
	enum TRACK_DATA_TYPE
	{
		TRACK_DATA_TYPE_AUDIO,
		TRACK_DATA_TYPE_MODE1_2048,
		TRACK_DATA_TYPE_MODE2_2352,
	};

	typedef std::shared_ptr<Framework::CStream> StreamPtr;

	COpticalMedia(const StreamPtr&);

	//TODO: Get Track Count
	TRACK_DATA_TYPE   GetTrackDataType(uint32) const;
	CISO9660*         GetFileSystem();

private:
	typedef std::unique_ptr<CISO9660> Iso9660Ptr;

	TRACK_DATA_TYPE m_track0DataType = TRACK_DATA_TYPE_MODE1_2048;
	Iso9660Ptr      m_fileSystem;
};
