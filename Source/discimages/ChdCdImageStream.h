#pragma once

#include "ChdImageStream.h"

class CChdCdImageStream : public CChdImageStream
{
public:
	enum TRACK_TYPE
	{
		TRACK_TYPE_MODE1,
		TRACK_TYPE_MODE2_RAW,
	};
	
	CChdCdImageStream(Framework::CStream*);

	TRACK_TYPE GetTrack0Type() const;
	
private:
	void ReadMetadata();
	
	TRACK_TYPE m_track0Type = TRACK_TYPE_MODE1;
};
