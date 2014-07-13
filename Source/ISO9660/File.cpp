#include "File.h"

using namespace ISO9660;

CFile::CFile(CISO9660* iso, uint64 start, uint64 size)
: m_iso(iso)
, m_start(start)
, m_end(start + size)
, m_position(0)
, m_blockPosition(static_cast<uint32>(start / CISO9660::BLOCKSIZE))
{
	iso->ReadBlock(m_blockPosition, m_block);
}

CFile::~CFile()
{

}

void CFile::Seek(int64 amount, Framework::STREAM_SEEK_DIRECTION whence)
{
	switch(whence)
	{
	case Framework::STREAM_SEEK_SET:
		m_position = amount;
		break;
	case Framework::STREAM_SEEK_CUR:
		m_position += amount;
		break;
	case Framework::STREAM_SEEK_END:
		m_position = m_end + 1;
		break;
	}
}

uint64 CFile::Tell()
{
	if(IsEOF())
	{
		return m_end - m_start;
	}
	return m_position;
}

uint64 CFile::Read(void* data, uint64 length)
{
	if(length == 0) return 0;

	uint64 total = length;
	//Read what's remaining of this block
	while(1)
	{
		SyncBlock();
		uint64 blockPosition	= (m_start + m_position) % CISO9660::BLOCKSIZE;
		uint64 blockRemain		= CISO9660::BLOCKSIZE - blockPosition;
		uint64 toRead			= (length > blockRemain) ? (blockRemain) : (length);

		memcpy(data, m_block + blockPosition, static_cast<uint32>(toRead));

		m_position += toRead;
		length -= toRead;
		data = reinterpret_cast<uint8*>(data) + toRead;

		if(length == 0) break;
	}

	return total;
}

uint64 CFile::Write(const void* pData, uint64 nLength)
{
	return -1;
}

bool CFile::IsEOF()
{
	return ((m_start + m_position) > m_end);
}

void CFile::SyncBlock()
{
	uint32 position = static_cast<uint32>((m_start + m_position) / CISO9660::BLOCKSIZE);
	if(position == m_blockPosition) return;

	m_iso->ReadBlock(position, m_block);
	m_blockPosition = position;
}
