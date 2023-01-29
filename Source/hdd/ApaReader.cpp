#include "ApaReader.h"
#include <cassert>
#include <cstring>
#include "HddDefs.h"

using namespace Hdd;

CApaReader::CApaReader(Framework::CStream& stream)
    : m_stream(stream)
{
}

std::vector<APA_HEADER> CApaReader::GetPartitions()
{
	std::vector<APA_HEADER> result;
	uint32_t lba = 0;
	while(1)
	{
		APA_HEADER header;
		m_stream.Seek(lba * g_sectorSize, Framework::STREAM_SEEK_SET);
		m_stream.Read(&header, sizeof(APA_HEADER));
		assert(header.magic == APA_HEADER_MAGIC);
		result.push_back(header);
		lba = header.next;
		if(lba == 0) break;
	}
	return result;
}

bool CApaReader::TryFindPartition(const char* partitionId, APA_HEADER& partitionHeader)
{
	uint32_t lba = 0;
	while(1)
	{
		APA_HEADER header;
		m_stream.Seek(lba * g_sectorSize, Framework::STREAM_SEEK_SET);
		m_stream.Read(&header, sizeof(APA_HEADER));
		assert(header.magic == APA_HEADER_MAGIC);
		if(!strcmp(header.id, partitionId))
		{
			partitionHeader = header;
			return true;
		}
		lba = header.next;
		if(lba == 0) break;
	}
	return false;
}
