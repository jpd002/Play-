#pragma once

#include "ChdImageStream.h"

class CChdCdImageStream : public CChdImageStream
{
public:
	enum DATA_TYPE
	{
		DATA_TYPE_CD_MODE1,
		DATA_TYPE_CD_MODE2_RAW,
		DATA_TYPE_DVD,
	};

	struct TRACK
	{
		uint32 frames = 0;
	};

	CChdCdImageStream(std::unique_ptr<Framework::CStream>);

	DATA_TYPE GetDataType() const;
	const std::vector<TRACK>& GetTracks() const;

private:
	void ReadMetadata();

	DATA_TYPE m_dataType = DATA_TYPE_CD_MODE1;
	std::vector<TRACK> m_tracks;
};
