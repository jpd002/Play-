#pragma once

#include "Stream.h"
#include <vector>

typedef struct _chd_file chd_file;

class CChdImageStream : public Framework::CStream
{
public:
	CChdImageStream(Framework::CStream* baseStream);
	~CChdImageStream();

	virtual void Seek(int64 pos, Framework::STREAM_SEEK_DIRECTION whence) override;
	virtual uint64 Tell() override;
	virtual bool IsEOF() override;
	virtual uint64 Read(void* dest, uint64 bytes) override;
	virtual uint64 Write(const void* src, uint64 bytes) override;

private:
	uint64 GetTotalSize() const;

	Framework::CStream* m_baseStream = nullptr;
	chd_file* m_chd = nullptr;
	uint32 m_hunkCount = 0;
	uint32 m_hunkSize = 0;
	uint64 m_position = 0;
	std::vector<uint8> m_hunkBuffer;
	uint32 m_hunkBufferIdx = ~0U;
};
