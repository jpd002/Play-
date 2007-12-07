#include <assert.h>
#include "PathTable.h"
#include "PtrMacro.h"
#include "stricmp.h"

using namespace Framework;
using namespace ISO9660;

CPathTable::CPathTable(CStream* pStream, uint32 nAddress)
{
	CPathTableRecord* pRecord;
	pStream->Seek(nAddress * 0x800, STREAM_SEEK_SET);

	while(1)
	{
		pRecord = new CPathTableRecord(pStream);
		if(pRecord->GetNameLenght() == 0)
		{
			DELETEPTR(pRecord);
			break;
		}
		m_Record.Insert(pRecord, m_Record.Count());
	}
}

CPathTable::~CPathTable()
{
	while(m_Record.Count())
	{
		delete m_Record.Pull();
	}
}

uint32 CPathTable::GetDirectoryAddress(unsigned int nRecord)
{
	CPathTableRecord* pRecord;

	nRecord--;
	assert(nRecord < m_Record.Count());

	pRecord = m_Record.Find(nRecord);
	return pRecord->GetAddress();
}

unsigned int CPathTable::FindRoot()
{
	unsigned int nCount, i;
	CPathTableRecord* pRecord;

	nCount = m_Record.Count();
	for(i = 0; i < nCount; i++)
	{
		pRecord = m_Record.Find(i);
		if(pRecord == NULL) continue;
		if(pRecord->GetNameLenght() == 1) break;
	}

	return i + 1;
}

unsigned int CPathTable::FindDirectory(const char* sName, unsigned int nParent)
{
	unsigned int nCount, i;
	CPathTableRecord* pRecord;

	nCount = m_Record.Count();
	for(i = 0; i < nCount; i++)
	{
		pRecord = m_Record.Find(i);
		if(pRecord == NULL) continue;
		if(nParent != pRecord->GetParentRecord()) continue;
		if(stricmp(sName, pRecord->GetName())) continue;
		break;
	}

	if(i == nCount) 
	{
		return 0;
	}

	return i + 1;
}
