#include "File.h"
#include <cassert>
#include <cstring>
#include <climits>
#include <algorithm>

using namespace ISO9660;

CFile::CFile(CBlockProvider* blockProvider, uint64 start)
    : m_blockProvider(blockProvider)
    , m_start(start)
    , m_end(ULLONG_MAX)
{
	InitBlock();
}

CFile::CFile(CBlockProvider* blockProvider, uint64 start, uint64 size)
    : m_blockProvider(blockProvider)
    , m_start(start)
    , m_end(start + size)
{
	InitBlock();
}

void CFile::Seek(int64 amount, Framework::STREAM_SEEK_DIRECTION whence)
{
	uint64 size = (m_end != ULLONG_MAX) ? (m_end - m_start) : ULLONG_MAX;
	switch(whence)
	{
	case Framework::STREAM_SEEK_SET:
		m_position = amount;
		break;
	case Framework::STREAM_SEEK_CUR:
		m_position += amount;
		break;
	case Framework::STREAM_SEEK_END:
		//Cannot seek from end with unbounded files
		assert(m_end != ULLONG_MAX);
		m_position = size + amount;
		break;
	}
	m_position = std::max<int64>(m_position, 0);
	m_position = std::min<uint64>(m_position, size);
	m_isEof = false;
}

uint64 CFile::Tell()
{
	return m_position;
}

uint64 CFile::Read(void* data, uint64 length)
{
	if(length == 0) return 0;

	//Pre-condition
	assert(m_position <= (m_end - m_start));

	//Check if we're at the end of the file
	uint64 offset = m_start + m_position;
	if(offset >= m_end)
	{
		m_isEof = true;
		return 0;
	}

	uint64 remainFileSize = m_end - offset;
	length = std::min<uint64>(length, remainFileSize);

	uint64 total = length;
	//Read what's remaining of this block
	while(1)
	{
		SyncBlock();
		uint64 blockPosition = (m_start + m_position) % CBlockProvider::BLOCKSIZE;
		uint64 blockRemain = CBlockProvider::BLOCKSIZE - blockPosition;
		uint64 toRead = (length > blockRemain) ? (blockRemain) : (length);

		memcpy(data, m_block + blockPosition, static_cast<uint32>(toRead));

		m_position += toRead;
		length -= toRead;
		data = reinterpret_cast<uint8*>(data) + toRead;

		if(length == 0) break;
	}

	//Post-condition
	assert(m_position <= (m_end - m_start));

	return total;
}

uint64 CFile::Write(const void* pData, uint64 nLength)
{
	return -1;
}

bool CFile::IsEOF()
{
	return m_isEof;
}

void CFile::InitBlock()
{
	m_blockPosition = static_cast<uint32>(m_start / CBlockProvider::BLOCKSIZE);
	m_blockProvider->ReadBlock(m_blockPosition, m_block);
}

void CFile::SyncBlock()
{
	uint32 position = static_cast<uint32>((m_start + m_position) / CBlockProvider::BLOCKSIZE);
	if(position == m_blockPosition) return;

	m_blockProvider->ReadBlock(position, m_block);
	m_blockPosition = position;
}
