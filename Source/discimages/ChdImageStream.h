#pragma once

#include "Stream.h"
#include <vector>

typedef struct _chd_file chd_file;
typedef struct _core_file core_file;

class CChdImageStream : public Framework::CStream
{
public:
	enum TRACK_TYPE
	{
		TRACK_TYPE_MODE1,
		TRACK_TYPE_MODE2_RAW,
	};

	CChdImageStream(Framework::CStream* baseStream);
	~CChdImageStream();

	TRACK_TYPE GetTrack0Type() const;

	virtual void Seek(int64 pos, Framework::STREAM_SEEK_DIRECTION whence) override;
	virtual uint64 Tell() override;
	virtual bool IsEOF() override;
	virtual uint64 Read(void* dest, uint64 bytes) override;
	virtual uint64 Write(const void* src, uint64 bytes) override;

private:
	void ReadMetadata();
	uint64 GetTotalSize() const;

	Framework::CStream* m_baseStream = nullptr;
	core_file* m_file = nullptr;
	chd_file* m_chd = nullptr;
	uint32 m_hunkCount = 0;
	uint32 m_hunkSize = 0;
	uint64 m_position = 0;
	std::vector<uint8> m_hunkBuffer;
	uint32 m_hunkBufferIdx = ~0U;

	TRACK_TYPE m_track0Type = TRACK_TYPE_MODE1;
};
