#include <stdlib.h>
#include "PathTableRecord.h"
#include "PtrMacro.h"
#include "alloca_def.h"

using namespace Framework;
using namespace ISO9660;

CPathTableRecord::CPathTableRecord(CStream* pStream)
{
	pStream->Read(&m_nNameLength, 1);
	pStream->Read(&m_nExLength, 1);
	pStream->Read(&m_nLocation, 4);
	pStream->Read(&m_nParentDir, 2);

    {
        char* directoryTemp = reinterpret_cast<char*>(alloca(m_nNameLength + 1));
	    pStream->Read(directoryTemp, m_nNameLength);
	    directoryTemp[m_nNameLength] = 0;
        m_sDirectory = directoryTemp;
    }

	if(m_nNameLength & 1)
	{
		pStream->Seek(1, STREAM_SEEK_CUR);
	}
}

CPathTableRecord::~CPathTableRecord()
{

}

uint8 CPathTableRecord::GetNameLength() const
{
	return m_nNameLength;
}

uint32 CPathTableRecord::GetAddress() const
{
	return m_nLocation;
}

uint32 CPathTableRecord::GetParentRecord() const
{
	return m_nParentDir;
}

const char* CPathTableRecord::GetName() const
{
	return m_sDirectory.c_str();
}
