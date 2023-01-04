#include "ApaReader.h"
#include "ApaDefs.h"
#include "HddDefs.h"

using namespace Hdd;

CApaReader::CApaReader(Framework::CStream& stream)
	: m_stream(stream)
{
	
}

uint32 CApaReader::FindPartition(const char* partitionId)
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
			return header.start;
		}
		lba = header.next;
		if(lba == 0) break;
	}
	return -1;
}
