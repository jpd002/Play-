#include <cassert>
#include <cstring>
#include "S3ObjectStream.h"
#include "amazon/AmazonS3Client.h"
#include "Singleton.h"
#include "AppConfig.h"
#include "PathUtils.h"
#include "string_format.h"
#include "StdStreamUtils.h"
#include "Log.h"

#define PREF_S3_OBJECTSTREAM_PROVIDER "s3.objectstream.provider"
#define PREF_R2_OBJECTSTREAM_ACCOUNTKEYID "r2.objectstream.accountkeyid"
#define PREF_S3_OBJECTSTREAM_ACCESSKEYID "s3.objectstream.accesskeyid"
#define PREF_S3_OBJECTSTREAM_SECRETACCESSKEY "s3.objectstream.secretaccesskey"
#define CACHE_PATH "Play Data Files/s3objectstream_cache"

#define LOG_NAME "s3objectstream"

#define BUFFERSIZE 0x40000

CS3ObjectStream::CConfig::CConfig()
{
	CAppConfig::GetInstance().RegisterPreferenceString(PREF_S3_OBJECTSTREAM_ACCESSKEYID, "");
	CAppConfig::GetInstance().RegisterPreferenceString(PREF_S3_OBJECTSTREAM_SECRETACCESSKEY, "");
	CAppConfig::GetInstance().RegisterPreferenceString(PREF_R2_OBJECTSTREAM_ACCOUNTKEYID, "");
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_S3_OBJECTSTREAM_PROVIDER, CAmazonConfigs::AWS_S3);
}

CAmazonConfigs CS3ObjectStream::CConfig::GetConfigs()
{
	CAmazonConfigs configs;
	configs.accessKeyId = CAppConfig::GetInstance().GetPreferenceString(PREF_S3_OBJECTSTREAM_ACCESSKEYID);
	configs.secretAccessKey = CAppConfig::GetInstance().GetPreferenceString(PREF_S3_OBJECTSTREAM_SECRETACCESSKEY);
	configs.accountKeyId = CAppConfig::GetInstance().GetPreferenceString(PREF_R2_OBJECTSTREAM_ACCOUNTKEYID);
	configs.m_provider = static_cast<CAmazonConfigs::S3PROVIDER>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_S3_OBJECTSTREAM_PROVIDER));
	return configs;
}

CS3ObjectStream::CS3ObjectStream(const char* bucketName, const char* objectKey)
    : m_bucketName(bucketName)
    , m_objectKey(objectKey)
{
	m_buffer.resize(BUFFERSIZE);
	Framework::PathUtils::EnsurePathExists(GetCachePath());
	GetObjectInfo();
}

uint64 CS3ObjectStream::Read(void* buffer, uint64 size)
{
	assert(m_objectPosition <= m_objectSize);

	uint64 adjSize = std::min(size, m_objectSize - m_objectPosition);
	auto outBuffer = reinterpret_cast<uint8*>(buffer);

	while(adjSize != 0)
	{
		//Read if we're inside buffer size
		if((m_bufferPosition / BUFFERSIZE) == (m_objectPosition / BUFFERSIZE))
		{
			uint64 bufferOffset = m_objectPosition % BUFFERSIZE;
			uint64 remainSize = BUFFERSIZE - bufferOffset;
			assert(remainSize <= BUFFERSIZE);
			auto copySize = std::min(remainSize, adjSize);
			assert(copySize <= adjSize);
			memcpy(outBuffer, m_buffer.data() + bufferOffset, copySize);
			m_objectPosition += copySize;
			outBuffer += copySize;
			adjSize -= copySize;
		}
		else
		{
			SyncBuffer();
		}
	}

	assert(m_objectPosition <= m_objectSize);
	return size;
}

uint64 CS3ObjectStream::Write(const void*, uint64)
{
	throw std::runtime_error("Not supported.");
}

void CS3ObjectStream::Seek(int64 offset, Framework::STREAM_SEEK_DIRECTION direction)
{
	switch(direction)
	{
	case Framework::STREAM_SEEK_SET:
		assert(offset <= m_objectSize);
		m_objectPosition = offset;
		break;
	case Framework::STREAM_SEEK_CUR:
		m_objectPosition += offset;
		break;
	case Framework::STREAM_SEEK_END:
		m_objectPosition = m_objectSize;
		break;
	}
}

uint64 CS3ObjectStream::Tell()
{
	return m_objectPosition;
}

bool CS3ObjectStream::IsEOF()
{
	return (m_objectPosition == m_objectSize);
}

fs::path CS3ObjectStream::GetCachePath()
{
	return Framework::PathUtils::GetCachePath() / CACHE_PATH;
}

std::string CS3ObjectStream::GenerateReadCacheKey(const std::pair<uint64, uint64>& range) const
{
	return string_format("%s-%llu-%llu", m_objectEtag.c_str(), range.first, range.second);
}

static std::string TrimQuotes(std::string input)
{
	if(input.empty()) return input;
	if(input[0] == '"')
	{
		input = std::string(input.begin() + 1, input.end());
	}
	if(input.empty()) return input;
	if(input[input.size() - 1] == '"')
	{
		input = std::string(input.begin(), input.end() - 1);
	}
	return input;
}

void CS3ObjectStream::GetObjectInfo()
{
	//Obtain bucket region
	{
		CAmazonS3Client client(CConfig::GetInstance().GetConfigs());

		GetBucketLocationRequest request;
		request.bucket = m_bucketName;

		auto result = client.GetBucketLocation(request);
		m_bucketRegion = result.locationConstraint;
	}

	//Obtain object info
	{
		CAmazonS3Client client(CConfig::GetInstance().GetConfigs(), m_bucketRegion);

		HeadObjectRequest request;
		request.bucket = m_bucketName;
		request.key = m_objectKey;

		auto objectHeader = client.HeadObject(request);
		m_objectSize = objectHeader.contentLength;
		m_objectEtag = TrimQuotes(objectHeader.etag);
	}
}

void CS3ObjectStream::SyncBuffer()
{
	m_bufferPosition = (m_objectPosition / BUFFERSIZE) * BUFFERSIZE;

	uint64 size = std::min<uint64>(BUFFERSIZE, m_objectSize - m_bufferPosition);
	auto range = std::make_pair(m_bufferPosition, m_bufferPosition + size - 1);
	auto readCacheFilePath = GetCachePath() / GenerateReadCacheKey(range);

#ifdef _TRACEGET
	static FILE* output = fopen("getobject.log", "wb");
	fprintf(output, "%ld,%ld,%ld\r\n", range.first, range.second, size);
	fflush(output);
#endif

	bool cachedReadSucceeded =
	    [&]() {
		    try
		    {
			    if(fs::exists(readCacheFilePath))
			    {
				    auto readCacheFileStream = Framework::CreateInputStdStream(readCacheFilePath.native());
				    FRAMEWORK_MAYBE_UNUSED auto cacheRead = readCacheFileStream.Read(m_buffer.data(), size);
				    assert(cacheRead == size);
				    return true;
			    }
		    }
		    catch(const std::exception& exception)
		    {
			    //Not a problem if we failed to read cache
			    CLog::GetInstance().Print(LOG_NAME, "Failed to read cache: '%s'.\r\n", exception.what());
		    }
		    return false;
	    }();

	if(!cachedReadSucceeded)
	{
		assert(size > 0);
		CAmazonS3Client client(CConfig::GetInstance().GetConfigs(), m_bucketRegion);
		GetObjectRequest request;
		request.key = m_objectKey;
		request.bucket = m_bucketName;
		request.range = range;
		auto objectContent = client.GetObject(request);
		assert(objectContent.data.size() == size);
		memcpy(m_buffer.data(), objectContent.data.data(), size);

		try
		{
			auto readCacheFileStream = Framework::CreateOutputStdStream(readCacheFilePath.native());
			readCacheFileStream.Write(objectContent.data.data(), size);
		}
		catch(const std::exception& exception)
		{
			//Not a problem if we failed to write cache
			CLog::GetInstance().Print(LOG_NAME, "Failed to write cache: '%s'.\r\n", exception.what());
		}
	}
}
