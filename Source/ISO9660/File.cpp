#include "File.h"

using namespace Framework;
using namespace ISO9660;

CFile::CFile(CISO9660* pISO, uint64 nStart, uint64 nSize)
{
	m_pISO		= pISO;
	m_nStart	= nStart;
	m_nEnd		= nStart + nSize;
	m_nPosition = 0;

	m_nBlockPosition = (uint32)(m_nStart / CISO9660::BLOCKSIZE);
	pISO->ReadBlock(m_nBlockPosition, m_nBlock);
}

CFile::~CFile()
{

}

void CFile::Seek(int64 nAmount, STREAM_SEEK_DIRECTION nWhence)
{
	switch(nWhence)
	{
	case Framework::STREAM_SEEK_SET:
		m_nPosition = nAmount;
		break;
	case Framework::STREAM_SEEK_CUR:
		m_nPosition += nAmount;
		break;
	case Framework::STREAM_SEEK_END:
		m_nPosition = m_nEnd + 1;
		break;
	}
}

uint64 CFile::Tell()
{
	if(IsEOF())
	{
		return m_nEnd - m_nStart;
	}
	return m_nPosition;
}

uint64 CFile::Read(void* pData, uint64 nLength)
{
	uint64 nBlockPosition, nBlockRemain, nToRead, nTotal;

	if(nLength == 0) return 0;

	nTotal = nLength;
	//Read what's remaining of this block
	while(1)
	{
		SyncBlock();
		nBlockPosition	= (m_nStart + m_nPosition) % CISO9660::BLOCKSIZE;
		nBlockRemain	= CISO9660::BLOCKSIZE - nBlockPosition;
		nToRead			= (nLength > nBlockRemain) ? (nBlockRemain) : (nLength);

		memcpy(pData, m_nBlock + nBlockPosition, (uint32)nToRead);

		m_nPosition += nToRead;
		nLength -= nToRead;
		pData = (uint8*)pData + nToRead;

		if(nLength == 0) break;
	}

	return nTotal;
}

uint64 CFile::Write(const void* pData, uint64 nLength)
{
	return -1;
}

bool CFile::IsEOF()
{
	return ((m_nStart + m_nPosition) > m_nEnd);
}

void CFile::SyncBlock()
{
	uint32 nPosition;
	
	nPosition = (uint32)((m_nStart + m_nPosition) / CISO9660::BLOCKSIZE);
	if(nPosition == m_nBlockPosition) return;

	m_pISO->ReadBlock(nPosition, m_nBlock);
	m_nBlockPosition = nPosition;
}
