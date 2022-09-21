#include "McDumpReader.h"
#include <cstring>

//Source:
//http://www.csclub.uwaterloo.ca:11068/mymc/ps2mcfs.html

static const char g_magic[29] = "Sony PS2 Memory Card Format ";

CMcDumpReader::CMcDumpReader(Framework::CStream& stream)
: m_stream(stream)
{
	m_stream.Read(&m_header, sizeof(HEADER));
	if(memcmp(m_header.magic, g_magic, sizeof(m_header.magic)))
	{
		throw std::exception();
	}
	
	assert(m_header.pageSize == 0x200);
	assert(m_header.pagesPerCluster == 2);
}

CMcDumpReader::Directory CMcDumpReader::ReadDirectory(uint32 dirCluster)
{
	std::vector<DIRENTRY> result;
	CFatReader reader(m_stream, m_header, dirCluster);
	DIRENTRY baseDirEntry = {};
	uint32 readAmount = reader.Read(&baseDirEntry, sizeof(DIRENTRY));
	assert(readAmount == sizeof(DIRENTRY));
	result.push_back(baseDirEntry);
	assert(baseDirEntry.length >= 2);
	for(uint32 i = 0; i < (baseDirEntry.length - 1); i++)
	{
		DIRENTRY dirEntry = {};
		readAmount = reader.Read(&dirEntry, sizeof(DIRENTRY));
		assert(readAmount == sizeof(DIRENTRY));
		result.push_back(dirEntry);
	}
	return result;
}

CMcDumpReader::Directory CMcDumpReader::ReadRootDirectory()
{
	return ReadDirectory(m_header.rootDirCluster);
}

std::vector<uint8> CMcDumpReader::ReadFile(uint32 fileCluster, uint32 fileSize)
{
	std::vector<uint8> result;
	CFatReader reader(m_stream, m_header, fileCluster);
	result.resize(fileSize);
	uint32 readAmount = reader.Read(result.data(), fileSize);
	assert(readAmount == fileSize);
	return result;
}

CMcDumpReader::CFatReader::CFatReader(Framework::CStream& stream, const HEADER& header, uint32 cluster)
: m_stream(stream)
, m_header(header)
{
	assert((m_header.pageSize * m_header.pagesPerCluster) == CLUSTER_SIZE);
	ReadFatCluster(cluster);
}

uint32 CMcDumpReader::CFatReader::Read(void* buffer, uint32 size)
{
	if(m_cluster == INVALID_CLUSTER_IDX)
	{
		return 0;
	}
	uint32 amountRead = 0;
	uint8* byteBuffer = reinterpret_cast<uint8*>(buffer);
	while(1)
	{
		if(m_bufferIndex == CLUSTER_SIZE)
		{
			uint32 entry = GetNextFatClusterEntry(m_cluster);
			static const uint32 nextValidBit = (1 << 31);
			if(entry & nextValidBit)
			{
				ReadFatCluster(entry & ~nextValidBit);
			}
			else
			{
				m_cluster = INVALID_CLUSTER_IDX;
				break;
			}
		}
		assert(m_bufferIndex < CLUSTER_SIZE);
		uint32 amountAvail = CLUSTER_SIZE - m_bufferIndex;
		uint32 toRead = std::min(amountAvail, size);
		memcpy(byteBuffer, m_buffer + m_bufferIndex, toRead);
		m_bufferIndex += toRead;
		byteBuffer += toRead;
		amountRead += toRead;
		size -= toRead;
		if(size == 0)
		{
			break;
		}
	}
	return amountRead;
}

void CMcDumpReader::CFatReader::ReadFatCluster(uint32 clusterIndex)
{
	m_cluster = clusterIndex;
	uint32 offset = (m_cluster + m_header.allocOffset) * CLUSTER_SIZE;
	m_stream.Seek(offset, Framework::STREAM_SEEK_SET);
	m_stream.Read(m_buffer, CLUSTER_SIZE);
	m_bufferIndex = 0;
}

uint32 CMcDumpReader::CFatReader::GetNextFatClusterEntry(uint32 clusterIndex)
{
	uint32 fatOffset = clusterIndex & 0xFF;
	uint32 indirectIndex = clusterIndex / 256;
	uint32 indirectOffset = indirectIndex & 0xFF;
	uint32 dblIndirectIndex = indirectIndex / 256;
	uint32 indirectClusterNum = m_header.ifcList[dblIndirectIndex];
	
	m_stream.Seek((indirectClusterNum * CLUSTER_SIZE) + (indirectOffset * 4), Framework::STREAM_SEEK_SET);
	uint32 fatClusterNum = m_stream.Read32();
	m_stream.Seek((fatClusterNum * CLUSTER_SIZE) + (fatOffset * 4), Framework::STREAM_SEEK_SET);
	uint32 entry = m_stream.Read32();
	return entry;
}
