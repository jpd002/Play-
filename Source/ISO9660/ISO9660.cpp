#include <limits.h>
#include "ISO9660.h"
#include "PtrMacro.h"
#include "StdStream.h"
#include "File.h"
#include "DirectoryRecord.h"
#include "stricmp.h"

using namespace Framework;
using namespace ISO9660;
using namespace std;

CISO9660::CISO9660(CStream* pStream) :
m_pStream(pStream),
m_volumeDescriptor(pStream),
m_pathTable(pStream, m_volumeDescriptor.GetLPathTableAddress())
{

}

CISO9660::~CISO9660()
{
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
	unsigned int nRecord;
	unsigned int nAddress;

	//Remove the first '/'
	if(sFilename[0] == '/' || sFilename[0] == '\\') sFilename++;

	nRecord = m_pathTable.FindRoot();

	while(1)
	{
		//Find the next '/'
		sNext = strchr(sFilename, '/');
		if(sNext == NULL) break;

        string sDir(sFilename, sNext);
		nRecord = m_pathTable.FindDirectory(sDir.c_str(), nRecord);
		if(nRecord == 0)
		{
			return false;
		}

		sFilename = sNext + 1;
	}

	nAddress = m_pathTable.GetDirectoryAddress(nRecord);

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
