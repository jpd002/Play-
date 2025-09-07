#include "ChdCdImageStream.h"
#include <libchdr/chd.h>
#include <cassert>
#include <cstring>
#include <stdexcept>

#define DVD_METADATA_TAG CHD_MAKE_TAG('D', 'V', 'D', ' ')

CChdCdImageStream::CChdCdImageStream(std::unique_ptr<Framework::CStream> baseStream)
    : CChdImageStream(std::move(baseStream))
{
	ReadMetadata();
}

CChdCdImageStream::DATA_TYPE CChdCdImageStream::GetDataType() const
{
	return m_dataType;
}

const std::vector<CChdCdImageStream::TRACK>& CChdCdImageStream::GetTracks() const
{
	return m_tracks;
}

void CChdCdImageStream::ReadMetadata()
{
	static const size_t bufferSize = 256;
	char metadata[bufferSize];
	uint32_t outlen = 0;
	if(chd_get_metadata(m_chd, CDROM_TRACK_METADATA2_TAG, 0, &metadata, sizeof(metadata), &outlen, nullptr, nullptr) == CHDERR_NONE)
	{
		auto getTrackDataType = [](const char* typeString) {
			if(!strcmp(typeString, "AUDIO"))
			{
				return DATA_TYPE_CD_AUDIO;
			}
			if(!strcmp(typeString, "MODE1_RAW"))
			{
				return DATA_TYPE_CD_MODE1_RAW;
			}
			if(!strcmp(typeString, "MODE2_RAW"))
			{
				return DATA_TYPE_CD_MODE2_RAW;
			}
			return DATA_TYPE_CD_MODE1;
		};
		assert(m_unitSize == 2448);
		int trackIndex = 0;
		int trackPos = 0;
		while(1)
		{
			metadata[outlen] = 0;
			int track = 0, frames = 0, preGap = 0, postGap = 0;
			char type[bufferSize], subType[bufferSize], pgType[bufferSize], pgSub[bufferSize];
			if(sscanf(metadata, CDROM_TRACK_METADATA2_FORMAT, &track, type, subType, &frames, &preGap, pgType, pgSub, &postGap) == 8)
			{
				if(track != (trackIndex + 1))
				{
					throw std::runtime_error("Inconsistent track order.");
				}
				type[bufferSize - 1] = 0;
				DATA_TYPE trackDataType = getTrackDataType(type);
				if(trackIndex == 0)
				{
					m_dataType = trackDataType;
				}
				m_tracks.push_back({static_cast<uint32>(trackPos), static_cast<uint32>(frames)});
				trackPos += frames;
				//CHD tracks start on a 4 sector boundary
				trackPos = (trackPos + 0x03) & ~0x03;
			}
			trackIndex++;
			chd_error result = chd_get_metadata(m_chd, CDROM_TRACK_METADATA2_TAG, trackIndex, &metadata, sizeof(metadata), &outlen, nullptr, nullptr);
			if(result != CHDERR_NONE)
			{
				break;
			}
		}
	}
	else if(chd_get_metadata(m_chd, DVD_METADATA_TAG, 0, &metadata, sizeof(metadata), &outlen, nullptr, nullptr) == CHDERR_NONE)
	{
		assert(m_unitSize == 2048);
		m_dataType = DATA_TYPE_DVD;
	}
	else
	{
		//No interesting metadata found, assuming MODE1 CD
		assert(m_unitSize == 2448);
		m_dataType = DATA_TYPE_CD_MODE1;
	}
}
