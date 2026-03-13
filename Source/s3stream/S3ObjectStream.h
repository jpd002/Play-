#pragma once

#include <vector>
#include "Singleton.h"
#include "Stream.h"
#include "filesystem_def.h"
#include "amazon/AmazonS3Client.h"

class CS3ObjectStream : public Framework::CStream
{
public:
	class CConfig : public CSingleton<CConfig>
	{
	public:
		CConfig();
		CAmazonConfigs GetConfigs();
	};

	CS3ObjectStream(const char*, const char*);

	uint64 Read(void*, uint64) override;
	uint64 Write(const void*, uint64) override;
	void Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
	uint64 Tell() override;
	bool IsEOF() override;

private:
	static fs::path GetCachePath();
	std::string GenerateReadCacheKey(const std::pair<uint64, uint64>&) const;
	void GetObjectInfo();
	void SyncBuffer();

	std::string m_bucketName;
	std::string m_bucketRegion;
	std::string m_objectKey;

	//Object Metadata
	uint64 m_objectSize = 0;
	std::string m_objectEtag;

	uint64 m_objectPosition = 0;

	std::vector<uint8> m_buffer;
	uint64 m_bufferPosition = ~0ULL;
};
