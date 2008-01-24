#include <fcntl.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <stdexcept>
#include "Posix_VolumeStream.h"

using namespace Framework::Posix;
using namespace std;

CVolumeStream::CVolumeStream(const char* volumePath)
{
	m_fd = open(volumePath, O_RDONLY);
	if(m_fd < 0) 
	{
		throw runtime_error("Couldn't open volume for reading.");
	}
	if(ioctl(m_fd, DKIOCGETBLOCKSIZE, &m_sectorSize) < 0)
	{
		throw runtime_error("Can't get sector size.");
	}
	m_cache = malloc(m_sectorSize);
	m_cacheSector = m_sectorSize - 1;
}

CVolumeStream::~CVolumeStream()
{
	close(m_fd);
	free(m_cache);
}

void CVolumeStream::Seek(int64 distance, STREAM_SEEK_DIRECTION from)
{
	switch(from)
	{
	case STREAM_SEEK_SET:
		m_position = distance;
		break;
	case STREAM_SEEK_CUR:
		m_position += distance;
		break;
	case STREAM_SEEK_END:
		throw runtime_error("Unsupported operation.");
		break;
	}
}

uint64 CVolumeStream::Tell()
{
	return m_position;
}

uint64 CVolumeStream::Read(void* buffer, uint64 size)
{
	uint64 retSize = size;
	uint8* dst = reinterpret_cast<uint8*>(buffer);
	uint8* src = reinterpret_cast<uint8*>(m_cache);
	
	while(size != 0)
	{
		SyncCache();
		
		size_t sectorOffset = static_cast<size_t>(m_position & (m_sectorSize - 1));
		size_t sectorRemain = static_cast<size_t>(m_sectorSize - sectorOffset);
		size_t copy = min<size_t>(size, sectorRemain);
		
		memcpy(dst, src + sectorOffset, copy);
		m_position += copy;
		size -= copy;
		dst += copy;
	}
	
	return retSize;
}

uint64 CVolumeStream::Write(const void* buffer, uint64 size)
{
	throw runtime_error("Not supported.");
}

bool CVolumeStream::IsEOF()
{
	return false;
}

void CVolumeStream::SyncCache()
{
	uint64 sectorPosition = m_position & ~(m_sectorSize - 1);
	if(sectorPosition == m_cacheSector) return;
	m_cacheSector = sectorPosition;
	lseek(m_fd, sectorPosition, SEEK_SET);
	read(m_fd, m_cache, m_sectorSize);
}
