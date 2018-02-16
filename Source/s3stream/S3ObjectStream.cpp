#include <cassert>
#include <cstring>
#include "S3ObjectStream.h"
#include "AmazonS3Client.h"
#include "Singleton.h"
#include "AppConfig.h"

#define PREF_S3_OBJECTSTREAM_ACCESSKEYID "s3.objectstream.accesskeyid"
#define PREF_S3_OBJECTSTREAM_SECRETACCESSKEY "s3.objectstream.secretaccesskey"

class CS3Config : public CSingleton<CS3Config>
{
public:
	CS3Config()
	{
		CAppConfig::GetInstance().RegisterPreferenceString(PREF_S3_OBJECTSTREAM_ACCESSKEYID, "");
		CAppConfig::GetInstance().RegisterPreferenceString(PREF_S3_OBJECTSTREAM_SECRETACCESSKEY, "");
	}

	std::string GetAccessKeyId()
	{
		return CAppConfig::GetInstance().GetPreferenceString(PREF_S3_OBJECTSTREAM_ACCESSKEYID);
	}

	std::string GetSecretAccessKey()
	{
		return CAppConfig::GetInstance().GetPreferenceString(PREF_S3_OBJECTSTREAM_SECRETACCESSKEY);
	}
};

CS3ObjectStream::CS3ObjectStream(const char* bucketName, const char* objectName)
    : m_bucketName(bucketName)
    , m_objectName(objectName)
{
	GetObjectInfo();
}

uint64 CS3ObjectStream::Read(void* buffer, uint64 size)
{
	assert(size > 0);
	CAmazonS3Client client(CS3Config::GetInstance().GetAccessKeyId(), CS3Config::GetInstance().GetSecretAccessKey(), m_bucketRegion);
	GetObjectRequest request;
	request.object = m_objectName;
	request.bucket = m_bucketName;
	request.range = std::make_pair(m_objectPosition, m_objectPosition + size - 1);
	auto objectContent = client.GetObject(request);
	assert(objectContent.data.size() == size);
	memcpy(buffer, objectContent.data.data(), size);
	m_objectPosition += size;

#ifdef _TRACEGET
	static FILE* output = fopen("getobject.log", "wb");
	fprintf(output, "%ld,%ld,%ld\r\n",
	    request.range.first, request.range.second, size);
	fflush(output);
#endif

	return 0;
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

void CS3ObjectStream::GetObjectInfo()
{
#if 0
	if(false)
	{
		CAmazonS3Client client(accessKeyId, secretAccessKey);

		GetBucketLocationRequest request;
		request.bucket = "ps2bootables";

		client.GetBucketLocation(request);
	}
#else
	m_bucketRegion = "us-west-2";
#endif

	CAmazonS3Client client(CS3Config::GetInstance().GetAccessKeyId(), CS3Config::GetInstance().GetSecretAccessKey(), m_bucketRegion);
	GetBucketLocationRequest request;
	request.bucket = m_bucketName;
	auto objectHeader = client.HeadObject(m_objectName);
	m_objectSize = objectHeader.contentLength;
}
