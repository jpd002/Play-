#include <assert.h>
#include <string.h>
#include <stdexcept>
#include "ELF.h"
#include "PtrStream.h"

using namespace Framework;
using namespace std;

CELF::CELF(uint8* content) :
m_content(content)
{
    CPtrStream stream(m_content, -1);

	stream.Read(&m_Header, sizeof(ELFHEADER));
	
	if(m_Header.nId[0] != 0x7F || m_Header.nId[1] != 'E' || m_Header.nId[2] != 'L' || m_Header.nId[3] != 'F')
	{
		throw runtime_error("This file isn't a valid ELF file.");
	}

	if(m_Header.nId[4] != 1 || m_Header.nId[5] != 1)
	{
		throw runtime_error("This ELF file format is not supported. Only 32-bits LSB ordered ELFs are supported.");
	}

    {
	    unsigned int nCount = m_Header.nProgHeaderCount;
	    m_pProgram = new ELFPROGRAMHEADER[nCount];

	    stream.Seek(m_Header.nProgHeaderStart, Framework::STREAM_SEEK_SET);
	    for(unsigned int i = 0; i < nCount; i++)
	    {
		    stream.Read(&m_pProgram[i], sizeof(ELFPROGRAMHEADER));
	    }
    }

    {
	    unsigned int nCount = m_Header.nSectHeaderCount;
	    m_pSection = new ELFSECTIONHEADER[nCount];

	    stream.Seek(m_Header.nSectHeaderStart, Framework::STREAM_SEEK_SET);
	    for(unsigned int i = 0; i < nCount; i++)
	    {
		    stream.Read(&m_pSection[i], sizeof(ELFSECTIONHEADER));
	    }
    }
}

CELF::~CELF()
{
	delete [] m_pProgram;
	delete [] m_pSection;
}

uint8* CELF::GetContent() const
{
    return m_content;
}

const ELFHEADER& CELF::GetHeader() const
{
    return m_Header;
}

ELFSECTIONHEADER* CELF::GetSection(unsigned int nIndex)
{
	if(nIndex >= m_Header.nSectHeaderCount)
	{
		return NULL;
	}
	return &m_pSection[nIndex];
}

const void* CELF::GetSectionData(unsigned int nIndex)
{
	ELFSECTIONHEADER* pSect = GetSection(nIndex);
	if(pSect == NULL) return NULL;
	return m_content + pSect->nOffset;
}

const void* CELF::FindSectionData(const char* sName)
{
	ELFSECTIONHEADER* pSect = FindSection(sName);
	if(pSect == NULL) return NULL;
	return m_content + pSect->nOffset;
}

ELFSECTIONHEADER* CELF::FindSection(const char* sName)
{
	const char* sSectData = (const char*)GetSectionData(m_Header.nSectHeaderStringTableIndex);
	if(sSectData == NULL) return NULL;
	for(unsigned int i = 0; i < m_Header.nSectHeaderCount; i++)
	{
		ELFSECTIONHEADER* pTemp = GetSection(i);
		const char* sSectName = sSectData + pTemp->nStringTableIndex;
		if(!strcmp(sName, sSectName))
		{
			return pTemp;
		}
	}
	return NULL;
}

ELFPROGRAMHEADER* CELF::GetProgram(unsigned int nIndex)
{
	if(nIndex >= m_Header.nProgHeaderCount)
	{
		return NULL;
	}
	return &m_pProgram[nIndex];
}
