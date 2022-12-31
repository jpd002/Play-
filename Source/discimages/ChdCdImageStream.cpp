#include "ChdCdImageStream.h"
#include <libchdr/chd.h>

CChdCdImageStream::CChdCdImageStream(std::unique_ptr<Framework::CStream> baseStream)
	: CChdImageStream(std::move(baseStream))
{
	//We only support 2448 bytes units
	assert(m_unitSize == 2448);

	ReadMetadata();
}

CChdCdImageStream::TRACK_TYPE CChdCdImageStream::GetTrack0Type() const
{
	return m_track0Type;
}

void CChdCdImageStream::ReadMetadata()
{
	static const size_t bufferSize = 256;
	char metadata[bufferSize];
	UINT32 outlen = 0;
	if(chd_get_metadata(m_chd, CDROM_TRACK_METADATA2_TAG, 0, &metadata, sizeof(metadata), &outlen, nullptr, nullptr) == CHDERR_NONE)
	{
		metadata[outlen] = 0;

		int track = 0, frames = 0, preGap = 0, postGap = 0;
		char type[bufferSize], subType[bufferSize], pgType[bufferSize], pgSub[bufferSize];
		if(sscanf(metadata, CDROM_TRACK_METADATA2_FORMAT, &track, type, subType, &frames, &preGap, pgType, pgSub, &postGap) == 8)
		{
			type[bufferSize - 1] = 0;
			if(!strcmp(type, "MODE2_RAW"))
			{
				m_track0Type = TRACK_TYPE_MODE2_RAW;
			}
			else
			{
				m_track0Type = TRACK_TYPE_MODE1;
			}
		}
	}
}
