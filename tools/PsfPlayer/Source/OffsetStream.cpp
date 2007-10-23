#include "OffsetStream.h"
#include <stdexcept>

using namespace Framework;
using namespace std;

COffsetStream::COffsetStream(CStream& stream, uint64 offset) :
m_stream(stream),
m_offset(offset)
{

}

COffsetStream::~COffsetStream()
{

}

uint64 COffsetStream::Tell()
{
    return m_stream.Tell() - m_offset;
}

void COffsetStream::Seek(int64 position, STREAM_SEEK_DIRECTION from)
{
    m_stream.Seek(position + m_offset, from);
}

uint64 COffsetStream::Read(void* buffer, uint64 size)
{
    return m_stream.Read(buffer, size);
}

uint64 COffsetStream::Write(const void* buffer, uint64 size)
{
    return m_stream.Write(buffer, size);
}

bool COffsetStream::IsEOF()
{
    throw runtime_error("Unsupported operation.");
}
