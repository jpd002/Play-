#include "DirectoryRecord.h"

using namespace ISO9660;

CDirectoryRecord::CDirectoryRecord()
{
	memset(&m_name, 0, sizeof(m_name));
}

CDirectoryRecord::CDirectoryRecord(Framework::CStream* stream)
{
	m_length = stream->Read8();
	m_exLength = stream->Read8();
	m_position = stream->Read32();
	stream->Seek(4, Framework::STREAM_SEEK_CUR);
	m_dataLength = stream->Read32();
	stream->Seek(4, Framework::STREAM_SEEK_CUR);
	stream->Seek(7, Framework::STREAM_SEEK_CUR);
	m_flags = stream->Read8();
	stream->Seek(6, Framework::STREAM_SEEK_CUR);
	uint8 nameSize = stream->Read8();
	stream->Read(m_name, nameSize);
	m_name[nameSize] = 0x00;

	int skipAmount = m_length - (0x21 + nameSize);
	if(skipAmount > 0)
	{
		stream->Seek(skipAmount, Framework::STREAM_SEEK_CUR);
	}
}

CDirectoryRecord::~CDirectoryRecord()
{
	
}

uint8 CDirectoryRecord::GetLength() const
{
	return m_length;
}

bool CDirectoryRecord::IsDirectory() const
{
	return (m_flags & 0x02) != 0;
}

const char* CDirectoryRecord::GetName() const
{
	return m_name;
}

uint32 CDirectoryRecord::GetPosition() const
{
	return m_position;
}

uint32 CDirectoryRecord::GetDataLength() const
{
	return m_dataLength;
}
