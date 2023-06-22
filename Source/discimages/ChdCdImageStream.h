#pragma once

#include "ChdImageStream.h"

class CChdCdImageStream : public CChdImageStream
{
public:
	enum TRACK_TYPE
	{
		TRACK_TYPE_CD_MODE1,
		TRACK_TYPE_CD_MODE2_RAW,
		TRACK_TYPE_DVD,
	};

	CChdCdImageStream(std::unique_ptr<Framework::CStream>);

	TRACK_TYPE GetTrack0Type() const;

private:
	void ReadMetadata();

	TRACK_TYPE m_track0Type = TRACK_TYPE_CD_MODE1;
};
