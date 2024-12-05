#pragma once

#include <vector>
#include <map>
#include "Stream.h"

class CMultiImageStream : public Framework::CStream
{
public:
	using StreamPtr = std::shared_ptr<Framework::CStream>;
	
	CMultiImageStream(std::vector<StreamPtr>);
	
	void Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
	uint64 Tell() override;
	uint64 Read(void* buffer, uint64 size) override;
	uint64 Write(const void*, uint64) override;
	bool IsEOF() override;
	
private:
	struct STREAM_INFO
	{
		StreamPtr stream;
		uint64 baseAddress = 0;
	};
	
	std::map<uint64, STREAM_INFO> m_streamMapping;
	uint64 m_totalSize = 0;
	uint64 m_position = 0;
};

