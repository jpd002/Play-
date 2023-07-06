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

bool CPfsReader::TryGetInodeFromPath(const char* path, PFS_INODE& resultInode)
{
	assert(path[0] == '/');
	path++;
	auto sections = StringUtils::Split(path, '/');

	auto sectionInode = ReadInode(m_superBlock.rootBlock.number, m_superBlock.rootBlock.subPart);
	for(unsigned int i = 0; i < sections.size(); i++)
	{
		const auto& section = sections[i];
		CPfsDirectoryReader dirReader(*this, m_stream, sectionInode);
		bool found = false;
		do
		{
			std::string entryName;
			PFS_INODE entryInode = {};
			dirReader.ReadEntry(entryName, entryInode);
			if(!strcmp(entryName.c_str(), section.c_str()))
			{
				sectionInode = entryInode;
				found = true;
				break;
			}
		} while(!dirReader.IsDone());
		if(!found)
		{
			return false;
		}
	}

	resultInode = sectionInode;
	return true;
}

CPfsFileReader* CPfsReader::GetFileStream(const char* path)
{
	PFS_INODE pathInode = {};
	if(!TryGetInodeFromPath(path, pathInode))
	{
		return nullptr;
	}
	return new CPfsFileReader(*this, m_stream, pathInode);
}

CPfsDirectoryReader* CPfsReader::GetDirectoryReader(const char* path)
{
	PFS_INODE pathInode = {};
	if(!TryGetInodeFromPath(path, pathInode))
	{
		return nullptr;
	}
	return new CPfsDirectoryReader(*this, m_stream, pathInode);
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
	assert((inode.mode & 0xF000) == 0x2000);
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

CPfsDirectoryReader::CPfsDirectoryReader(CPfsReader& reader, Framework::CStream& stream, PFS_INODE inode)
    : m_reader(reader)
{
	assert((inode.mode & 0xF000) == 0x1000);
	assert(inode.dataCount == 2);

	uint32 dirLba = reader.GetBlockLba(inode.data[1].number, inode.data[1].subPart);

	stream.Seek(dirLba * g_sectorSize, Framework::STREAM_SEEK_SET);
	stream.Read(m_dirBlock, sizeof(m_dirBlock));
	m_dirBlockCurr = m_dirBlock;
	m_dirBlockEnd = m_dirBlock + g_dirBlockSize;

	Advance();
}

void CPfsDirectoryReader::ReadEntry(std::string& entryName, PFS_INODE& entryInode)
{
	assert(!IsDone());
	entryName = m_currentEntryName;
	entryInode = m_currentEntryInode;
	Advance();
}

bool CPfsDirectoryReader::IsDone() const
{
	return m_dirBlockCurr == m_dirBlockEnd;
}

void CPfsDirectoryReader::Advance()
{
	assert((m_dirBlockEnd - m_dirBlockCurr) >= sizeof(PFS_DIRENTRY));
	auto dirEntry = reinterpret_cast<const PFS_DIRENTRY*>(m_dirBlockCurr);
	uint32_t entryLength = dirEntry->allocatedLength & 0xFFF;
	if(entryLength == 0)
	{
		//We're done
		m_dirBlockCurr = m_dirBlockEnd;
		return;
	}
	assert((m_dirBlockEnd - m_dirBlockCurr) >= entryLength);
	std::string entryName;
	for(int i = 0; i < dirEntry->pathLength; i++)
	{
		entryName += m_dirBlockCurr[sizeof(PFS_DIRENTRY) + i];
	}
	m_dirBlockCurr += entryLength;

	m_currentEntryName = entryName;
	m_currentEntryInode = m_reader.ReadInode(dirEntry->inode, dirEntry->subPart);
}
