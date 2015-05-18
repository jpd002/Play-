#include <assert.h>
#include "PathTable.h"
#include "File.h"
#include "stricmp.h"

using namespace ISO9660;

CPathTable::CPathTable(CBlockProvider* blockProvider, uint32 tableLba)
{
	CFile stream(blockProvider, static_cast<uint64>(tableLba) * CBlockProvider::BLOCKSIZE);
	while(1)
	{
		CPathTableRecord record(stream);
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

uint32 CPathTable::GetDirectoryAddress(unsigned int recordIndex) const
{
	recordIndex--;
	auto recordIterator(m_records.find(recordIndex));

	if(recordIterator == m_records.end())
	{
		throw std::exception();
	}
	const CPathTableRecord& record(recordIterator->second);
	return record.GetAddress();
}

unsigned int CPathTable::FindRoot() const
{
	for(const auto& recordPair : m_records)
	{
		const auto& record = recordPair.second;
		if(record.GetNameLength() == 1)
		{
			return static_cast<unsigned int>(recordPair.first) + 1;
		}
	}

	return 0;
}

unsigned int CPathTable::FindDirectory(const char* sName, unsigned int nParent) const
{
	for(const auto& recordPair : m_records)
	{
		const auto& record = recordPair.second;
		if(record.GetParentRecord() != nParent) continue;
		if(stricmp(sName, record.GetName())) continue;
		return static_cast<unsigned int>(recordPair.first) + 1;
	}

	return 0;
}
