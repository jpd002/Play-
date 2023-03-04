#include "PfsReader.h"
#include <cassert>
#include <cstring>
#include "HddDefs.h"
#include "StringUtils.h"

using namespace Hdd;

static uint32_t GetScale(uint32_t num, uint32_t size)
{
	uint32 scale = 0;
	while((size << scale) != num)
	{
		scale++;
	}
	return scale;
}

CPfsReader::CPfsReader(Framework::CStream& stream, const APA_HEADER& partitionHeader)
    : m_stream(stream)
    , m_partitionHeader(partitionHeader)
{
	uint32_t superBlockLba = partitionHeader.start + PFS_SUPERBLOCK_LBA;
	m_stream.Seek(superBlockLba * g_sectorSize, Framework::STREAM_SEEK_SET);
	m_stream.Read(&m_superBlock, sizeof(PFS_SUPERBLOCK));
	assert(m_superBlock.magic == PFS_SUPERBLOCK_MAGIC);
	m_inodeScale = GetScale(m_superBlock.zoneSize, PFS_INODE_SIZE);
}

CPfsFileReader* CPfsReader::GetFileStream(const char* path)
{
	assert(path[0] == '/');
	path++;
	auto sections = StringUtils::Split(path, '/');

	auto sectionInode = ReadInode(m_superBlock.rootBlock.number, m_superBlock.rootBlock.subPart);
	for(unsigned int i = 0; i < sections.size(); i++)
	{
		assert((sectionInode.mode & 0xF000) == 0x1000);
		assert(sectionInode.dataCount == 2);
		uint32 dirLba = GetBlockLba(sectionInode.data[1].number, sectionInode.data[1].subPart);

		static const uint32_t dirBlockSize = g_sectorSize << PFS_BLOCK_SCALE;
		const auto& section = sections[i];
		uint8 dirBlock[dirBlockSize];
		m_stream.Seek(dirLba * g_sectorSize, Framework::STREAM_SEEK_SET);
		m_stream.Read(dirBlock, sizeof(dirBlock));
		auto dirBlockCurr = dirBlock;
		auto dirBlockEnd = dirBlock + dirBlockSize;
		while(1)
		{
			assert((dirBlockEnd - dirBlockCurr) >= sizeof(PFS_DIRENTRY));
			auto dirEntry = reinterpret_cast<const PFS_DIRENTRY*>(dirBlockCurr);
			uint32_t entryLength = dirEntry->allocatedLength & 0xFFF;
			if(entryLength == 0)
			{
				//We're done
				dirBlockCurr = dirBlockEnd;
				break;
			}
			assert((dirBlockEnd - dirBlockCurr) >= entryLength);
			std::string entryName;
			for(int i = 0; i < dirEntry->pathLength; i++)
			{
				entryName += dirBlockCurr[sizeof(PFS_DIRENTRY) + i];
			}
			if(!strcmp(entryName.c_str(), section.c_str()))
			{
				break;
			}
			dirBlockCurr += entryLength;
		}

		if(dirBlockCurr == dirBlockEnd)
		{
			//Not found
			return nullptr;
		}

		auto dirEntry = reinterpret_cast<const PFS_DIRENTRY*>(dirBlockCurr);
		sectionInode = ReadInode(dirEntry->inode, dirEntry->subPart);
	}

	assert((sectionInode.mode & 0xF000) == 0x2000);
	return new CPfsFileReader(*this, m_stream, sectionInode);
}

uint32 CPfsReader::GetZoneSize() const
{
	return m_superBlock.zoneSize;
}

uint32 CPfsReader::GetBlockLba(uint32 number, uint32 subPart) const
{
	uint32 baseLba = 0;
	if(subPart == 0)
	{
		baseLba = m_partitionHeader.start;
	}
	else
	{
		subPart--;
		assert(subPart < m_partitionHeader.subPartCount);
		baseLba = m_partitionHeader.subParts[subPart].start;
	}
	return baseLba + ((number << m_inodeScale) << PFS_BLOCK_SCALE);
}

PFS_INODE CPfsReader::ReadInode(uint32 number, uint32 subPart)
{
	PFS_INODE result = {};
	uint32 inodeLba = GetBlockLba(number, subPart);
	m_stream.Seek(inodeLba * g_sectorSize, Framework::STREAM_SEEK_SET);
	m_stream.Read(&result, sizeof(PFS_INODE));
	assert(result.magic == PFS_INODE_SEGDESC_DIRECT_MAGIC);
	return result;
}

CPfsFileReader::CPfsFileReader(CPfsReader& reader, Framework::CStream& stream, PFS_INODE inode)
    : m_reader(reader)
    , m_stream(stream)
    , m_inode(inode)
{
}

void CPfsFileReader::Seek(int64 position, Framework::STREAM_SEEK_DIRECTION whence)
{
	uint64 fileSize = m_inode.size;
	switch(whence)
	{
	case Framework::STREAM_SEEK_SET:
		m_position = std::min<uint64>(position, fileSize);
		break;
	case Framework::STREAM_SEEK_CUR:
		m_position = std::min<uint64>(m_position + position, fileSize);
		break;
	case Framework::STREAM_SEEK_END:
		m_position = fileSize;
		break;
	}
	m_isEof = false;
}

uint64 CPfsFileReader::Tell()
{
	return m_position;
}

uint64 CPfsFileReader::Read(void* buffer, uint64 length)
{
	uint64 fileSize = m_inode.size;
	if(m_position >= fileSize)
	{
		m_isEof = true;
		return 0;
	}

	uint64 remainFileSize = fileSize - m_position;
	length = std::min<uint64>(length, remainFileSize);

	uint64 zoneSize = m_reader.GetZoneSize();
	assert((zoneSize % g_sectorSize) == 0);
	assert(m_inode.dataCount >= 2);

	uint64 segmentPosition = m_position;
	uint32 segmentIndex = 1;
	for(uint32 i = 1; i < m_inode.dataCount; i++)
	{
		uint64_t segmentSize = m_inode.data[i].count * zoneSize;
		if(segmentPosition < segmentSize)
		{
			segmentIndex = i;
			break;
		}
		segmentPosition -= segmentSize;
	}

	uint8* charBuffer = reinterpret_cast<uint8*>(buffer);
	uint64 readRemain = length;
	while(readRemain != 0)
	{
		assert(segmentIndex < m_inode.dataCount);
		uint64 segmentSize = m_inode.data[segmentIndex].count * zoneSize;
		uint64 segmentRemain = segmentSize - segmentPosition;
		uint64 blockPosition = segmentPosition % g_sectorSize;
		uint64 blockRemain = g_sectorSize - blockPosition;
		uint32 segmentLba = m_reader.GetBlockLba(m_inode.data[segmentIndex].number, m_inode.data[segmentIndex].subPart);
		uint64 toRead = std::min<uint64>(blockRemain, readRemain);
		m_stream.Seek((segmentLba * g_sectorSize) + segmentPosition, Framework::STREAM_SEEK_SET);
		m_stream.Read(charBuffer, toRead);
		readRemain -= toRead;
		charBuffer += toRead;
		segmentPosition += toRead;
		if(segmentPosition >= segmentSize)
		{
			segmentPosition -= segmentSize;
			segmentIndex++;
		}
	}

	m_position += length;
	return length;
}

uint64 CPfsFileReader::Write(const void*, uint64)
{
	assert(false);
	return 0;
}

bool CPfsFileReader::IsEOF()
{
	return m_isEof;
}
