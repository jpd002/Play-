#include "DirectoryRecord.h"

using namespace Framework;
using namespace ISO9660;

CDirectoryRecord::CDirectoryRecord()
{

}

CDirectoryRecord::CDirectoryRecord(CStream* pStream)
{
    uint8 nNameSize;
    int nSkip;

    pStream->Read(&m_nLength,		1);
    pStream->Read(&m_nExLength,		1);
    pStream->Read(&m_nPosition,		4);
    pStream->Seek(4, STREAM_SEEK_CUR);
    pStream->Read(&m_nDataLength,	4);
    pStream->Seek(4, STREAM_SEEK_CUR);
    pStream->Seek(7, STREAM_SEEK_CUR);
    pStream->Read(&m_nFlags,		1);
    pStream->Seek(6, STREAM_SEEK_CUR);
    pStream->Read(&nNameSize, 1);
    pStream->Read(m_sName, nNameSize);
    m_sName[nNameSize] = 0x00;

    nSkip = m_nLength - (0x21 + nNameSize);
    if(nSkip > 0)
    {
        pStream->Seek(nSkip, STREAM_SEEK_CUR);
    }
}

CDirectoryRecord::~CDirectoryRecord()
{
	
}

uint8 CDirectoryRecord::GetLength() const
{
    return m_nLength;
}

bool CDirectoryRecord::IsDirectory() const
{
    return (m_nFlags & 0x02) != 0;
}

const char* CDirectoryRecord::GetName() const
{
    return m_sName;
}

uint32 CDirectoryRecord::GetPosition() const
{
    return m_nPosition;
}

uint32 CDirectoryRecord::GetDataLength() const
{
    return m_nDataLength;
}
