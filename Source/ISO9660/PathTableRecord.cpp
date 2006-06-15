#include <malloc.h>
#include "PathTableRecord.h"
#include "PtrMacro.h"

using namespace Framework;
using namespace ISO9660;

CPathTableRecord::CPathTableRecord(CStream* pStream)
{
	pStream->Read(&m_nNameLenght, 1);
	pStream->Read(&m_nExLenght, 1);
	pStream->Read(&m_nLocation, 4);
	pStream->Read(&m_nParentDir, 2);

	m_sDirectory = (char*)malloc(m_nNameLenght + 1);
	pStream->Read(m_sDirectory, m_nNameLenght);
	m_sDirectory[m_nNameLenght] = 0;

	if(m_nNameLenght & 1)
	{
		pStream->Seek(1, STREAM_SEEK_CUR);
	}
}

CPathTableRecord::~CPathTableRecord()
{
	FREEPTR(m_sDirectory);
}

uint8 CPathTableRecord::GetNameLenght() const
{
	return m_nNameLenght;
}

uint32 CPathTableRecord::GetAddress() const
{
	return m_nLocation;
}

uint32 CPathTableRecord::GetParentRecord() const
{
	return m_nParentDir;
}

const char* CPathTableRecord::GetName() const
{
	return m_sDirectory;
}
