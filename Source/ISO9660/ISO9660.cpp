#include <limits.h>
#include "ISO9660.h"
#include "PtrMacro.h"
#include "StdStream.h"
#include "File.h"
#include "DirectoryRecord.h"
#include "stricmp.h"

using namespace Framework;
using namespace ISO9660;

CISO9660::CISO9660(CStream* pStream)
{
	m_pStream = pStream;
	m_pVolumeDescriptor = new CVolumeDescriptor(pStream);
	m_pPathTable = new CPathTable(pStream, m_pVolumeDescriptor->GetLPathTableAddress());
}

CISO9660::~CISO9660()
{
	DELETEPTR(m_pVolumeDescriptor);
	DELETEPTR(m_pPathTable);
	DELETEPTR(m_pStream);
}

void CISO9660::ReadBlock(uint32 nAddress, void* pData)
{
	//Caching mechanism?
	m_pStream->Seek(nAddress * BLOCKSIZE, STREAM_SEEK_SET);
	m_pStream->Read(pData, BLOCKSIZE);
}

bool CISO9660::GetFileRecord(CDirectoryRecord* pRecord, const char* sFilename)
{
	const char* sNext;
	char sDir[257];
	unsigned int nRecord;
	unsigned int nAddress;
	size_t nLenght;

	//Remove the first '/'
	if(sFilename[0] == '/' || sFilename[0] == '\\') sFilename++;

	nRecord = m_pPathTable->FindRoot();

	while(1)
	{
		//Find the next '/'
		sNext = strchr(sFilename, '/');
		if(sNext == NULL) break;

		nLenght = sNext - sFilename;
		strncpy(sDir, sFilename, nLenght);
		sDir[nLenght] = 0x00;

		nRecord = m_pPathTable->FindDirectory(sDir, nRecord);
		if(nRecord == 0)
		{
			return false;
		}

		sFilename = sNext + 1;
	}

	nAddress = m_pPathTable->GetDirectoryAddress(nRecord);

	return GetFileRecordFromDirectory(pRecord, nAddress, sFilename);
}

bool CISO9660::GetFileRecordFromDirectory(CDirectoryRecord* pRecord, uint32 nAddress, const char* sFilename)
{
	CFile Directory(this, nAddress * BLOCKSIZE, ULLONG_MAX - (nAddress * BLOCKSIZE));

	while(1)
	{
		CDirectoryRecord Entry(&Directory);

		if(Entry.GetLength() == 0) break;
		if(Entry.IsDirectory()) continue;
		if(strnicmp(Entry.GetName(), sFilename, strlen(sFilename))) continue;

		(*pRecord) = Entry;
		return true;
	}

	return false;
}

CStream* CISO9660::Open(const char* sFilename)
{
	CDirectoryRecord Record;

	if(GetFileRecord(&Record, sFilename))
	{
		return new CFile(this, Record.GetPosition() * BLOCKSIZE, Record.GetDataLength());
	}

	return NULL;
}
