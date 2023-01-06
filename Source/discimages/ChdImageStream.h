#pragma once

#include "Stream.h"
#include <vector>
#include <memory>

typedef struct _chd_file chd_file;
typedef struct chd_core_file core_file;

class CChdImageStream : public Framework::CStream
{
public:
	CChdImageStream(std::unique_ptr<Framework::CStream> baseStream);
	virtual ~CChdImageStream();

	uint32 GetUnitSize() const;

	virtual void Seek(int64 pos, Framework::STREAM_SEEK_DIRECTION whence) override;
	virtual uint64 Tell() override;
	virtual bool IsEOF() override;
	virtual uint64 Read(void* dest, uint64 bytes) override;
	virtual uint64 Write(const void* src, uint64 bytes) override;

protected:
	uint64 GetTotalSize() const;

	std::unique_ptr<Framework::CStream> m_baseStream;
	core_file* m_file = nullptr;
	chd_file* m_chd = nullptr;
	uint32 m_unitSize = 0;
	uint32 m_hunkCount = 0;
	uint32 m_hunkSize = 0;
	uint64 m_position = 0;
	std::vector<uint8> m_hunkBuffer;
	uint32 m_hunkBufferIdx = ~0U;
};
