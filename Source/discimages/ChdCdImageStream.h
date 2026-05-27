#pragma once

#include "ChdImageStream.h"

class CChdCdImageStream : public CChdImageStream
{
public:
	enum DATA_TYPE
	{
		DATA_TYPE_CD_AUDIO,
		DATA_TYPE_CD_MODE1,
		DATA_TYPE_CD_MODE1_RAW,
		DATA_TYPE_CD_MODE2_RAW,
		DATA_TYPE_DVD,
	};

	struct TRACK
	{
		uint32 startFrame = 0;
		uint32 frames = 0;
	};

	CChdCdImageStream(std::unique_ptr<Framework::CStream>);

	DATA_TYPE GetDataType() const;
	const std::vector<TRACK>& GetTracks() const;

	uint64 Read(void*, uint64) override;

private:
	void ReadMetadata();

	DATA_TYPE m_dataType = DATA_TYPE_CD_MODE1;
	std::vector<TRACK> m_tracks;

	uint64 m_audioRangeStart = -1;
	uint64 m_audioRangeEnd = -1;
};
