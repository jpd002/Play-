#include "MultiImageStream.h"
#include <cassert>

CMultiImageStream::CMultiImageStream(std::vector<StreamPtr> streams)
{
	for(auto& stream  : streams)
	{
		uint64 streamSize = stream->GetLength();
		STREAM_INFO streamInfo;
		streamInfo.stream = std::move(stream);
		streamInfo.baseAddress = m_totalSize;
		m_totalSize += streamSize;
		m_streamMapping.insert(std::make_pair(m_totalSize, std::move(streamInfo)));
	}
}

void CMultiImageStream::Seek(int64 position, Framework::STREAM_SEEK_DIRECTION direction)
{
	switch(direction)
	{
	case Framework::STREAM_SEEK_CUR:
		m_position = std::min<uint64>(m_position + position, m_totalSize);
		break;
	case Framework::STREAM_SEEK_SET:
		m_position = std::min<uint64>(position, m_totalSize);
		break;
	case Framework::STREAM_SEEK_END:
		m_position = m_totalSize;
		break;
	}
}
	
uint64 CMultiImageStream::Tell()
{
	return m_position;
}

uint64 CMultiImageStream::Read(void* buffer, uint64 size)
{
	auto typedBuffer = reinterpret_cast<uint8*>(buffer);
	while(size != 0)
	{
		auto currentStreamIterator = m_streamMapping.lower_bound(m_position);
		if(currentStreamIterator->first == m_position)
		{
			currentStreamIterator++;
		}
		assert(currentStreamIterator != std::end(m_streamMapping));
		assert(m_position < currentStreamIterator->first);
		const auto& streamInfo = currentStreamIterator->second;
		uint64 maxReadSize = currentStreamIterator->first - m_position;
		assert(m_position >= streamInfo.baseAddress);
		uint64 readBase = m_position - streamInfo.baseAddress;
		streamInfo.stream->Seek(readBase, Framework::STREAM_SEEK_SET);
		uint64 readSize = std::min(maxReadSize, size);
		uint64 result = streamInfo.stream->Read(typedBuffer, readSize);
		assert(result == readSize);
		size -= readSize;
	}
}
	
uint64 CMultiImageStream::Write(const void*, uint64)
{
	throw std::runtime_error("Not supported.");
}
	
bool CMultiImageStream::IsEOF()
{
	return false;
}
