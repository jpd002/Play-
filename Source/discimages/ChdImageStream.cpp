#include "ChdImageStream.h"
#include <libchdr/chd.h>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include "ChdStreamSupport.h"

CChdImageStream::CChdImageStream(Framework::CStream* baseStream)
    : m_baseStream(baseStream)
{
	m_file = ChdStreamSupport::CreateFileFromStream(baseStream);
	chd_error result = chd_open_file(m_file, CHD_OPEN_READ, nullptr, &m_chd);
	if(result != CHDERR_NONE)
	{
		core_ffree(m_file);
		throw std::runtime_error("Failed to open CHD file.");
	}
	auto header = chd_get_header(m_chd);
	//We only support 2448 bytes units
	assert(header->unitbytes == 2448);
	m_hunkCount = header->hunkcount;
	m_hunkSize = header->hunkbytes;
	m_hunkBuffer.resize(m_hunkSize);

	ReadMetadata();
}

CChdImageStream::~CChdImageStream()
{
	chd_close(m_chd);
	core_ffree(m_file);
}

CChdImageStream::TRACK_TYPE CChdImageStream::GetTrack0Type() const
{
	return m_track0Type;
}

void CChdImageStream::ReadMetadata()
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

void CChdImageStream::Seek(int64 position, Framework::STREAM_SEEK_DIRECTION origin)
{
	switch(origin)
	{
	case Framework::STREAM_SEEK_CUR:
		m_position += position;
		break;
	case Framework::STREAM_SEEK_SET:
		m_position = position;
		break;
	case Framework::STREAM_SEEK_END:
		m_position = GetTotalSize() + position;
		break;
	}
}

uint64 CChdImageStream::Tell()
{
	return m_position;
}

bool CChdImageStream::IsEOF()
{
	return m_position >= GetTotalSize();
}

uint64 CChdImageStream::Read(void* buffer, uint64 size)
{
	uint32 hunkPosition = m_position % m_hunkSize;
	uint32 hunkRemainSize = m_hunkSize - hunkPosition;
	assert((hunkPosition + size) <= m_hunkSize);
	uint32 hunkIdx = m_position / m_hunkSize;
	if(hunkIdx != m_hunkBufferIdx)
	{
		chd_read(m_chd, hunkIdx, m_hunkBuffer.data());
		m_hunkBufferIdx = hunkIdx;
	}
	memcpy(buffer, m_hunkBuffer.data() + hunkPosition, size);
	m_position += size;
	return size;
}

uint64 CChdImageStream::Write(const void* buffer, uint64 size)
{
	throw std::runtime_error("Not supported.");
}

uint64 CChdImageStream::GetTotalSize() const
{
	return m_hunkCount * m_hunkSize;
}
