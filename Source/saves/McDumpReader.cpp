#include "McDumpReader.h"
#include <cstring>
#include <cassert>
#include <stdexcept>
#include "maybe_unused.h"

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

	//Some cards have ECC which are not taken in consideration by pageSize.
	//Compute raw page size which contains ECC codes

	uint32 totalPageCount = m_header.pagesPerCluster * m_header.clustersPerCard;
	size_t expectedSize = m_header.pageSize * totalPageCount;
	size_t actualSize = m_stream.GetLength();
	assert(actualSize >= expectedSize);

	uint32 pageSpareSize = (actualSize - expectedSize) / totalPageCount;

	//Some other card dumps have ECC but have a wrong size which makes the above check fail
	//If we have 0 at 0x20C, we most likely have a dump that has ECC interleaved with pages
	//Problematic dumps: Smash Court Pro Tournament
	{
		m_stream.Seek(0x20C, Framework::STREAM_SEEK_SET);
		uint32 spareAreaCheck = m_stream.Read32();
		if(spareAreaCheck == 0)
		{
			pageSpareSize = 0x10;
		}
	}

	m_rawPageSize = m_header.pageSize + pageSpareSize;
}

CMcDumpReader::Directory CMcDumpReader::ReadDirectory(uint32 dirCluster)
{
	std::vector<DIRENTRY> result;
	CFatReader reader(*this, dirCluster);
	DIRENTRY baseDirEntry = {};
	FRAMEWORK_MAYBE_UNUSED uint32 readAmount = reader.Read(&baseDirEntry, sizeof(DIRENTRY));
	assert(readAmount == sizeof(DIRENTRY));
	result.push_back(baseDirEntry);
	assert(baseDirEntry.length >= 2);
	for(uint32 i = 0; i < (baseDirEntry.length - 1); i++)
	{
		DIRENTRY dirEntry = {};
		readAmount = reader.Read(&dirEntry, sizeof(DIRENTRY));
		if(readAmount != sizeof(DIRENTRY))
		{
			throw std::runtime_error("Failed to read directory entry.");
		}
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
	CFatReader reader(*this, fileCluster);
	result.resize(fileSize);
	FRAMEWORK_MAYBE_UNUSED uint32 readAmount = reader.Read(result.data(), fileSize);
	assert(readAmount == fileSize);
	return result;
}

void CMcDumpReader::ReadCluster(uint32 clusterIndex, void* buffer)
{
	uint32 pageIndex = clusterIndex * m_header.pagesPerCluster;
	assert(m_rawPageSize >= m_header.pageSize);
	uint32 spareSize = m_rawPageSize - m_header.pageSize;
	m_stream.Seek(pageIndex * m_rawPageSize, Framework::STREAM_SEEK_SET);
	for(uint32 i = 0; i < m_header.pagesPerCluster; i++)
	{
		m_stream.Read(reinterpret_cast<uint8*>(buffer) + (i * m_header.pageSize), m_header.pageSize);
		m_stream.Seek(spareSize, Framework::STREAM_SEEK_CUR);
	}
}

void CMcDumpReader::ReadClusterCached(uint32 clusterIndex, void* buffer)
{
	static const uint32 clusterSize = m_header.pagesPerCluster * m_header.pageSize;
	auto clusterIterator = m_clusterCache.find(clusterIndex);
	if(clusterIterator == std::end(m_clusterCache))
	{
		Cluster cluster;
		cluster.resize(clusterSize);
		ReadCluster(clusterIndex, cluster.data());
		auto result = m_clusterCache.emplace(std::make_pair(clusterIndex, std::move(cluster)));
		clusterIterator = result.first;
	}

	const auto& cluster = clusterIterator->second;
	assert(cluster.size() == clusterSize);
	memcpy(buffer, cluster.data(), cluster.size());
}

CMcDumpReader::CFatReader::CFatReader(CMcDumpReader& parent, uint32 cluster)
    : m_parent(parent)
{
	assert(m_parent.m_header.pageSize == MC_PAGE_SIZE);
	assert(m_parent.m_header.pagesPerCluster == MC_PAGES_PER_CLUSTER);
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
		if(m_bufferIndex == MC_CLUSTER_SIZE)
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
		assert(m_bufferIndex < MC_CLUSTER_SIZE);
		uint32 amountAvail = MC_CLUSTER_SIZE - m_bufferIndex;
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
	m_parent.ReadCluster(m_cluster + m_parent.m_header.allocOffset, m_buffer);
	m_bufferIndex = 0;
}

uint32 CMcDumpReader::CFatReader::GetNextFatClusterEntry(uint32 clusterIndex)
{
	uint32 clusterTemp[MC_CLUSTER_SIZE / sizeof(uint32)];

	uint32 fatOffset = clusterIndex & 0xFF;
	uint32 indirectIndex = clusterIndex / 256;
	uint32 indirectOffset = indirectIndex & 0xFF;
	uint32 dblIndirectIndex = indirectIndex / 256;
	uint32 indirectClusterNum = m_parent.m_header.ifcList[dblIndirectIndex];

	m_parent.ReadClusterCached(indirectClusterNum, clusterTemp);
	uint32 fatClusterNum = clusterTemp[indirectOffset];

	m_parent.ReadClusterCached(fatClusterNum, clusterTemp);
	uint32 entry = clusterTemp[fatOffset];

	return entry;
}
