#include <stdlib.h>
#include "PathTableRecord.h"

using namespace ISO9660;

CPathTableRecord::CPathTableRecord(Framework::CStream& stream)
{
	m_nameLength = stream.Read8();
	m_exLength = stream.Read8();
	m_location = stream.Read32();
	m_parentDir = stream.Read16();
	m_directory = stream.ReadString(m_nameLength);

	if(m_nameLength & 1)
	{
		stream.Seek(1, Framework::STREAM_SEEK_CUR);
	}
}

CPathTableRecord::~CPathTableRecord()
{

}

uint8 CPathTableRecord::GetNameLength() const
{
	return m_nameLength;
}

uint32 CPathTableRecord::GetAddress() const
{
	return m_location;
}

uint32 CPathTableRecord::GetParentRecord() const
{
	return m_parentDir;
}

const char* CPathTableRecord::GetName() const
{
	return m_directory.c_str();
}
