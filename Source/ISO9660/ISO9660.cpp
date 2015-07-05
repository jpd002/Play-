#include <limits.h>
#include "ISO9660.h"
#include "StdStream.h"
#include "File.h"
#include "DirectoryRecord.h"
#include "stricmp.h"

using namespace ISO9660;

CISO9660::CISO9660(const BlockProviderPtr& blockProvider)
: m_blockProvider(blockProvider)
, m_volumeDescriptor(blockProvider.get())
, m_pathTable(blockProvider.get(), m_volumeDescriptor.GetLPathTableAddress())
{

}

CISO9660::~CISO9660()
{

}

void CISO9660::ReadBlock(uint32 address, void* data)
{
	//The buffer is needed to make sure exception handlers
	//are properly called as some system calls (ie.: ReadFile)
	//won't generate an exception when trying to write to
	//a write protected area
	m_blockProvider->ReadBlock(address, m_blockBuffer);
	memcpy(data, m_blockBuffer, CBlockProvider::BLOCKSIZE);
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
	CFile directory(m_blockProvider.get(), address * CBlockProvider::BLOCKSIZE);

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
		return new CFile(m_blockProvider.get(), static_cast<uint64>(record.GetPosition()) * CBlockProvider::BLOCKSIZE, record.GetDataLength());
	}

	return nullptr;
}
