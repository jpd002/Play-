#pragma once

#include "Stream.h"

class CJsDiscImageDeviceStream : public Framework::CStream
{
public:
	virtual ~CJsDiscImageDeviceStream() = default;

	void Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
	uint64 Tell() override;
	uint64 Read(void*, uint64) override;
	uint64 Write(const void*, uint64) override;
	bool IsEOF() override;
	void Flush() override;

private:
	uint64 m_position = 0;
};
