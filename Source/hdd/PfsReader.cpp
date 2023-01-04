#include "PfsReader.h"
#include "HddDefs.h"

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

CPfsReader::CPfsReader(Framework::CStream& stream, uint32 baseLba)
	: m_stream(stream)
	, m_baseLba(baseLba)
{
	uint32_t superBlockLba = m_baseLba + PFS_SUPERBLOCK_LBA;
	m_stream.Seek(superBlockLba * g_sectorSize, Framework::STREAM_SEEK_SET);
	m_stream.Read(&m_superBlock, sizeof(PFS_SUPERBLOCK));
	assert(m_superBlock.magic == PFS_SUPERBLOCK_MAGIC);
	m_inodeScale = GetScale(m_superBlock.zoneSize, PFS_INODE_SIZE);
}

CPfsFileReader* CPfsReader::GetFileStream(const char* path)
{
	assert(path[0] == '/');
	path++;
	assert(strchr(path, '/') == nullptr);
	
	auto rootInode = ReadInode(m_superBlock.rootBlock.number, m_superBlock.rootBlock.subPart);
	assert(rootInode.dataCount == 2);
	uint32 dirLba = GetBlockLba(rootInode.data[1].number, rootInode.data[1].subPart);
	
	static const uint32_t dirBlockSize = g_sectorSize << PFS_BLOCK_SCALE;
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
		if(!strcmp(entryName.c_str(), path))
		{
			break;
		}
		dirBlockCurr += entryLength;
	}
	
	if(dirBlockCurr == dirBlockEnd)
	{
		return nullptr;
	}
	
	auto dirEntry = reinterpret_cast<const PFS_DIRENTRY*>(dirBlockCurr);
	auto fileInode = ReadInode(dirEntry->inode, dirEntry->subPart);
	return new CPfsFileReader(*this, m_stream, fileInode);
}

uint32 CPfsReader::GetBlockLba(uint32 number, uint32 subPart)
{
	assert(subPart == 0);
	return m_baseLba + ((number << m_inodeScale) << PFS_BLOCK_SCALE);
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

uint64_t CPfsFileReader::Tell()
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

	assert(m_inode.dataCount == 2);
	uint32 fileLba = m_reader.GetBlockLba(m_inode.data[1].number, m_inode.data[1].subPart);

	m_stream.Seek(fileLba * g_sectorSize, Framework::STREAM_SEEK_SET);
	m_stream.Read(buffer, length);

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
