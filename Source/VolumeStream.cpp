#include <exception>
#include "VolumeStream.h"

using namespace Framework;
using namespace Framework::Win32;
using namespace std;

CVolumeStream::CVolumeStream(char nDriveLetter)
{
	char sPath[7] =
	    {
	        '\\',
	        '\\',
	        '.',
	        '\\',
	        nDriveLetter,
	        ':',
	        '\0'};

	m_nVolume = CreateFileA(sPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	m_nPosition = 0;

	m_nSectorSize = 0x800;
	m_nCacheSector = m_nSectorSize - 1;
	m_pCache = VirtualAlloc(NULL, (SIZE_T)m_nSectorSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

CVolumeStream::~CVolumeStream()
{
	VirtualFree(m_pCache, 0, MEM_RELEASE);
	CloseHandle(m_nVolume);
}

void CVolumeStream::Seek(int64 nDistance, STREAM_SEEK_DIRECTION nFrom)
{
	switch(nFrom)
	{
	case STREAM_SEEK_SET:
		m_nPosition = nDistance;
		break;
	case STREAM_SEEK_CUR:
		m_nPosition += nDistance;
		break;
	case STREAM_SEEK_END:
		throw exception("Operation not supported.");
		break;
	}
}

uint64 CVolumeStream::Tell()
{
	return m_nPosition;
}

uint64 CVolumeStream::Read(void* pBuffer, uint64 nSize)
{
	uint8* pDst;
	uint8* pSrc;
	uint64 nRetSize;

	pSrc = (uint8*)m_pCache;
	pDst = (uint8*)pBuffer;
	nRetSize = nSize;

	while(nSize != 0)
	{
		size_t nSectorRemain, nSectorOffset, nCopy;

		SyncCache();

		nSectorOffset = (size_t)(m_nPosition & (m_nSectorSize - 1));
		nSectorRemain = (size_t)(m_nSectorSize - nSectorOffset);
		nCopy = min((size_t)nSize, nSectorRemain);

		memcpy(pDst, pSrc + nSectorOffset, nCopy);

		m_nPosition += nCopy;
		nSize -= nCopy;
		pDst += nCopy;
	}

	return nRetSize;
}

uint64 CVolumeStream::Write(const void* pBuffer, uint64 nSize)
{
	throw exception("Operation not-supported.");
}

bool CVolumeStream::IsEOF()
{
	return false;
}

void CVolumeStream::SyncCache()
{
	uint64 nSectorPosition;
	DWORD nRead;

	nSectorPosition = m_nPosition & ~(m_nSectorSize - 1);
	if(nSectorPosition == m_nCacheSector) return;
	m_nCacheSector = nSectorPosition;

	SetFilePointer(m_nVolume, (uint32)nSectorPosition, (PLONG)((uint32*)&nSectorPosition) + 1, FILE_BEGIN);

	ReadFile(m_nVolume, m_pCache, (DWORD)m_nSectorSize, &nRead, NULL);
}
