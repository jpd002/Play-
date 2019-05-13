#pragma once

#include "Singleton.h"
#include "Stream.h"
#include "boost_filesystem_def.h"

class CS3ObjectStream : public Framework::CStream
{
public:
	class CConfig : public CSingleton<CConfig>
	{
	public:
		CConfig();
		std::string GetAccessKeyId();
		std::string GetSecretAccessKey();
	};

	CS3ObjectStream(const char*, const char*);

	uint64 Read(void*, uint64) override;
	uint64 Write(const void*, uint64) override;
	void Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
	uint64 Tell() override;
	bool IsEOF() override;

private:
	static boost::filesystem::path GetCachePath();
	std::string GenerateReadCacheKey(const std::pair<uint64, uint64>&) const;
	void GetObjectInfo();
	void SyncBuffer();

	std::string m_bucketName;
	std::string m_bucketRegion;
	std::string m_objectName;

	//Object Metadata
	uint64 m_objectSize = 0;
	std::string m_objectEtag;

	uint64 m_objectPosition = 0;

	std::vector<uint8> m_buffer;
	uint64 m_bufferPosition = ~0ULL;
};
