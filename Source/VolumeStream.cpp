#include <exception>
#include <cassert>
#include "tcharx.h"
#include "VolumeStream.h"

using namespace Framework;
using namespace Framework::Win32;

CVolumeStream::CVolumeStream(const TCHAR* volumePath)
{
	//We need to remove any trailing slashes to open volumes
	auto fixedVolumePath = std::tstring(volumePath);
	if(!fixedVolumePath.empty() && (*fixedVolumePath.rbegin() == '\\'))
	{
		fixedVolumePath = std::tstring(fixedVolumePath.begin(), fixedVolumePath.end() - 1);
	}

	m_nVolume = CreateFile(fixedVolumePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	assert(m_nVolume != INVALID_HANDLE_VALUE);
	if(m_nVolume == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to open volume.");
	}

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
		throw std::exception("Operation not supported.");
		break;
	}
}

uint64 CVolumeStream::Tell()
{
	return m_nPosition;
}

uint64 CVolumeStream::Read(void* pBuffer, uint64 nSize)
{
	auto pSrc = reinterpret_cast<uint8*>(m_pCache);
	auto pDst = reinterpret_cast<uint8*>(pBuffer);
	uint64 nRetSize = nSize;

	while(nSize != 0)
	{
		SyncCache();

		uint64 nSectorOffset = (m_nPosition & (m_nSectorSize - 1));
		uint64 nSectorRemain = (m_nSectorSize - nSectorOffset);
		uint64 nCopy = min(nSize, nSectorRemain);

		memcpy(pDst, pSrc + nSectorOffset, nCopy);

		m_nPosition += nCopy;
		nSize -= nCopy;
		pDst += nCopy;
	}

	return nRetSize;
}

uint64 CVolumeStream::Write(const void* pBuffer, uint64 nSize)
{
	throw std::exception("Operation not-supported.");
}

bool CVolumeStream::IsEOF()
{
	return false;
}

void CVolumeStream::SyncCache()
{
	uint64 nSectorPosition = m_nPosition & ~(m_nSectorSize - 1);
	if(nSectorPosition == m_nCacheSector) return;
	m_nCacheSector = nSectorPosition;

	SetFilePointer(m_nVolume, (uint32)nSectorPosition, (PLONG)((uint32*)&nSectorPosition) + 1, FILE_BEGIN);

	DWORD nRead = 0;
	BOOL success = ReadFile(m_nVolume, m_pCache, (DWORD)m_nSectorSize, &nRead, NULL);
	assert(success);
}
