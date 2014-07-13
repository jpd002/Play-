#include <limits.h>
#include "ISO9660.h"
#include "StdStream.h"
#include "File.h"
#include "DirectoryRecord.h"
#include "stricmp.h"

using namespace ISO9660;

CISO9660::CISO9660(Framework::CStream* stream)
: m_stream(stream)
, m_volumeDescriptor(stream)
, m_pathTable(stream, m_volumeDescriptor.GetLPathTableAddress())
{

}

CISO9660::~CISO9660()
{
	delete m_stream;
}

void CISO9660::ReadBlock(uint32 address, void* data)
{
	//Caching mechanism?
	m_stream->Seek(static_cast<uint64>(address) * BLOCKSIZE, Framework::STREAM_SEEK_SET);
	m_stream->Read(data, BLOCKSIZE);
}

bool CISO9660::GetFileRecord(CDirectoryRecord* record, const char* filename)
{
	//Remove the first '/'
	if(filename[0] == '/' || filename[0] == '\\') filename++;

	unsigned int recordIndex = m_pathTable.FindRoot();

	while(1)
	{
		//Find the next '/'
		const char* next = strchr(filename, '/');
		if(next == nullptr) break;

		std::string dir(filename, next);
		recordIndex = m_pathTable.FindDirectory(dir.c_str(), recordIndex);
		if(recordIndex == 0)
		{
			return false;
		}

		filename = next + 1;
	}

	unsigned int address = m_pathTable.GetDirectoryAddress(recordIndex);

	return GetFileRecordFromDirectory(record, address, filename);
}

bool CISO9660::GetFileRecordFromDirectory(CDirectoryRecord* record, uint32 address, const char* filename)
{
	CFile directory(this, address * BLOCKSIZE, ULLONG_MAX - (address * BLOCKSIZE));

	while(1)
	{
		CDirectoryRecord entry(&directory);

		if(entry.GetLength() == 0) break;
		if(strnicmp(entry.GetName(), filename, strlen(filename))) continue;

		(*record) = entry;
		return true;
	}

	return false;
}

Framework::CStream* CISO9660::Open(const char* filename)
{
	CDirectoryRecord record;

	if(GetFileRecord(&record, filename))
	{
		return new CFile(this, static_cast<uint64>(record.GetPosition()) * BLOCKSIZE, record.GetDataLength());
	}

	return nullptr;
}
