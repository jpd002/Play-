#include <assert.h>
#include "PathTable.h"
#include "PtrMacro.h"
#include "stricmp.h"

using namespace Framework;
using namespace ISO9660;
using namespace std;

CPathTable::CPathTable(CStream* pStream, uint32 nAddress)
{
	pStream->Seek(nAddress * 0x800, Framework::STREAM_SEEK_SET);

	while(1)
	{
        CPathTableRecord record(pStream);
		if(record.GetNameLength() == 0)
		{
			break;
		}
        m_records.insert(RecordMapType::value_type(m_records.size(), record));
	}
}

CPathTable::~CPathTable()
{

}

uint32 CPathTable::GetDirectoryAddress(unsigned int nRecord) const
{
	nRecord--;
    RecordMapType::const_iterator recordIterator(m_records.find(nRecord));

    if(recordIterator == m_records.end())
    {
        throw exception();
    }
    const CPathTableRecord& record(recordIterator->second);
	return record.GetAddress();
}

unsigned int CPathTable::FindRoot() const
{
    for(RecordMapType::const_iterator recordIterator(m_records.begin());
        m_records.end() != recordIterator; recordIterator++)
    {
        const CPathTableRecord& record(recordIterator->second);
        if(record.GetNameLength() == 1)
        {
            return static_cast<unsigned int>(recordIterator->first) + 1;
        }
    }

    return 0;
}

unsigned int CPathTable::FindDirectory(const char* sName, unsigned int nParent) const
{
    for(RecordMapType::const_iterator recordIterator(m_records.begin());
        m_records.end() != recordIterator; recordIterator++)
    {
        const CPathTableRecord& record(recordIterator->second);
        if(record.GetParentRecord() != nParent) continue;
		if(stricmp(sName, record.GetName())) continue;
        return static_cast<unsigned int>(recordIterator->first) + 1;
    }

    return 0;
}
