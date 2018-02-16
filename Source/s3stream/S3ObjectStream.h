#pragma once

#include "Stream.h"

class CS3ObjectStream : public Framework::CStream
{
public:
	CS3ObjectStream(const char*, const char*);

	uint64  Read(void*, uint64) override;
	uint64  Write(const void*, uint64) override;
	void    Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
	uint64  Tell() override;
	bool    IsEOF() override;

private:
	void GetObjectInfo();

	std::string m_bucketName;
	std::string m_bucketRegion;
	std::string m_objectName;
	uint64 m_objectSize = 0;
	uint64 m_objectPosition = 0;
};
