#pragma once

#include "Types.h"
#include "Stream.h"

class CCsoImageStream : public Framework::CStream
{
public:
	                    CCsoImageStream(Framework::CStream* baseStream);
	virtual             ~CCsoImageStream();

	virtual void        Seek(int64 pos, Framework::STREAM_SEEK_DIRECTION whence);
	virtual uint64      Tell();
	virtual bool        IsEOF();
	virtual uint64      Read(void* dest, uint64 bytes);
	virtual uint64      Write(const void* src, uint64 bytes);

private:
	void                ReadFileHeader();
	void                InitializeBuffers();
	uint64              GetTotalSize() const;
	uint32              ReadFromNextFrame(uint8* dest, uint64 maxBytes);
	uint64              ReadBaseAt(uint64 pos, uint8* dest, uint64 bytes);
	void                DecompressFrame(uint32 frame, uint64 readBufferSize);

	Framework::CStream* m_baseStream;
	uint32              m_frameSize;
	uint8               m_frameShift;
	uint8               m_indexShift;
	uint8*              m_readBuffer;
	uint8*              m_zlibBuffer;
	uint32              m_zlibBufferFrame;
	uint32*             m_index;
	uint64              m_totalSize;
	uint64              m_position;
};
