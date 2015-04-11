#include <algorithm>
#include <assert.h>
#include "CsoImageStream.h"
#include "zlib.h"

typedef uint32 uint32_le;
typedef uint64 uint64_le;

static const uint32 CSO_READ_BUFFER_SIZE = 256 * 1024;

struct CsoHeader
{
	uint8 magic[4];
	uint32_le header_size;
	uint64_le total_bytes;
	uint32_le frame_size;
	uint8 ver;
	uint8 align;
	uint8 reserved[2];
};

CCsoImageStream::CCsoImageStream(CStream *baseStream)
	: m_baseStream(baseStream), m_readBuffer(nullptr), m_zlibBuffer(nullptr), m_index(nullptr), m_position(0)
{
	if(baseStream == nullptr)
	{
		throw std::runtime_error("Null base stream supplied.");
	}

	ReadFileHeader();
	InitializeBuffers();
}

CCsoImageStream::~CCsoImageStream()
{
	delete [] m_readBuffer;
	delete [] m_zlibBuffer;
	delete [] m_index;
}

void CCsoImageStream::ReadFileHeader()
{
	CsoHeader hdr = {};

	m_baseStream->Seek(0, Framework::STREAM_SEEK_SET);
	if(m_baseStream->Read(&hdr, sizeof(CsoHeader)) != sizeof(CsoHeader))
	{
		throw std::runtime_error("Could not read full CSO header.");
	}

	if(hdr.magic[0] != 'C' || hdr.magic[1] != 'I' || hdr.magic[2] != 'S' || hdr.magic[3] != 'O')
	{
		throw std::runtime_error("Not a valid CSO file.");
	}
	if(hdr.ver > 1)
	{
		throw std::runtime_error("Only CSOv1 supported right now.");
	}

	m_frameSize = hdr.frame_size;
	if((m_frameSize & (m_frameSize - 1)) != 0)
	{
		throw std::runtime_error("CSO frame size must be a power of two.");
	}
	if(m_frameSize < 0x800)
	{
		throw std::runtime_error("CSO frame size must be at least one sector.");
	}

	// Determine the translation from bytes to frame.
	m_frameShift = 0;
	for(uint32 i = m_frameSize; i > 1; i >>= 1)
	{
		++m_frameShift;
	}

	m_indexShift = hdr.align;
	m_totalSize = hdr.total_bytes;
}

void CCsoImageStream::InitializeBuffers()
{
	uint32 numFrames = static_cast<uint32>((m_totalSize + m_frameSize - 1) / m_frameSize);

	// We might read a bit of alignment too, so be prepared.
	if(m_frameSize + (1 << m_indexShift) < CSO_READ_BUFFER_SIZE)
	{
		m_readBuffer = new uint8[CSO_READ_BUFFER_SIZE];
	}
	else
	{
		m_readBuffer = new uint8[m_frameSize + (1 << m_indexShift)];
	}
	m_zlibBuffer = new uint8[m_frameSize + (1 << m_indexShift)];
	m_zlibBufferFrame = numFrames;

	const uint32 indexSize = numFrames + 1;
	m_index = new uint32[indexSize];
	if(m_baseStream->Read(m_index, sizeof(uint32) * indexSize) != sizeof(uint32) * indexSize)
	{
		throw std::runtime_error("Unable to read CSO index.");
	}
}

void CCsoImageStream::Seek(int64 position, Framework::STREAM_SEEK_DIRECTION origin)
{
	switch (origin)
	{
	case Framework::STREAM_SEEK_CUR:
		m_position += position;
		break;
	case Framework::STREAM_SEEK_SET:
		m_position = position;
		break;
	case Framework::STREAM_SEEK_END:
		m_position = GetTotalSize() + position;
		break;
	}
}

uint64 CCsoImageStream::Tell()
{
	return m_position;
}

bool CCsoImageStream::IsEOF()
{
	return m_position >= GetTotalSize();
}

uint64 CCsoImageStream::Read(void *buffer, uint64 size)
{
	uint64 remaining = size;
	uint8 *dest = reinterpret_cast<uint8 *>(buffer);

	// We read only one frame at a time for now.
	while(remaining > 0 && !IsEOF())
	{
		uint32 bytes = ReadFromNextFrame(dest, remaining);
		remaining -= bytes;
		m_position += bytes;
		dest += bytes;
	}

	return size - remaining;
}

uint64 CCsoImageStream::Write(const void *buffer, uint64 size)
{
	throw std::exception("Unable to write to CSO, read only.");
}

uint64 CCsoImageStream::GetTotalSize() const
{
	return m_totalSize;
}

uint32 CCsoImageStream::ReadFromNextFrame(uint8 *dest, uint64 maxBytes)
{
	assert(!IsEOF());

	const uint32 frame = static_cast<uint32>(m_position >> m_frameShift);
	const uint32 offset = static_cast<uint32>(m_position - (frame << m_frameShift));
	// This is how many bytes we will actually be reading from this frame.
	const uint32 bytes = static_cast<uint32>(std::min(maxBytes, static_cast<uint64>(m_frameSize - offset)));

	// Grab the index data for the frame we're about to read.
	const bool compressed = (m_index[frame + 0] & 0x80000000) == 0;
	const uint32 index0 = m_index[frame + 0] & 0x7FFFFFFF;
	const uint32 index1 = m_index[frame + 1] & 0x7FFFFFFF;

	// Calculate where the compressed payload is (if compressed.)
	const uint64 frameRawPos = static_cast<uint64>(index0) << m_indexShift;
	const uint64 frameRawSize = (index1 - index0) << m_indexShift;

	if(!compressed)
	{
		// Just read directly, easy.
		if(ReadBaseAt(frameRawPos + offset, dest, bytes) != bytes)
		{
			throw std::exception("Unable to read uncompressed bytes from CSO.");
		}
	}
	else
	{
		// We don't need to decompress if we already did this same frame last time.
		if(m_zlibBufferFrame != frame)
		{
			// This might be less bytes than frameRawSize in case of padding on the last frame.
			// This is because the index positions must be aligned.
			const uint64 readRawBytes = ReadBaseAt(frameRawPos, m_readBuffer, frameRawSize);
			DecompressFrame(frame, readRawBytes);
		}

		// Now we just copy the offset data from the cache.
		memcpy(dest, m_zlibBuffer + offset, bytes);
	}

	return bytes;
}

void CCsoImageStream::DecompressFrame(uint32 frame, uint64 readBufferSize)
{
	z_stream z;
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;
	if(inflateInit2(&z, -15) != Z_OK)
	{
		throw std::exception("Unable to initialize zlib for CSO decompression.");
	}

	z.next_in = m_readBuffer;
	z.avail_in = static_cast<uint32>(readBufferSize);
	z.next_out = m_zlibBuffer;
	z.avail_out = m_frameSize;

	int status = inflate(&z, Z_FINISH);
	if(status != Z_STREAM_END || z.total_out != m_frameSize)
	{
		inflateEnd(&z);
		throw std::exception("Unable to decompress CSO frame using zlib.");
	}
	inflateEnd(&z);

	// Our buffer now contains this frame.
	m_zlibBufferFrame = frame;
}

uint64 CCsoImageStream::ReadBaseAt(uint64 pos, uint8 *dest, uint64 bytes)
{
	m_baseStream->Seek(pos, Framework::STREAM_SEEK_SET);
	return m_baseStream->Read(dest, bytes);
}
