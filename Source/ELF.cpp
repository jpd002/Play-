#include <assert.h>
#include <string.h>
#include <stdexcept>
#include "ELF.h"

using namespace Framework;
using namespace std;

CELF::CELF(CStream* pS)
{
	unsigned int nCount, i;

    pS->Seek(0, Framework::STREAM_SEEK_END);
	m_nLenght = (unsigned int)pS->Tell();

	m_pData = new char[m_nLenght];
	pS->Seek(0, Framework::STREAM_SEEK_SET);
	pS->Read(m_pData, m_nLenght);

    pS->Seek(0, Framework::STREAM_SEEK_SET);
	pS->Read(&m_Header, sizeof(ELFHEADER));
	
	if(m_Header.nId[0] != 0x7F || m_Header.nId[1] != 'E' || m_Header.nId[2] != 'L' || m_Header.nId[3] != 'F')
	{
		throw runtime_error("This file isn't a valid ELF file.");
	}

	if(m_Header.nId[4] != 1 || m_Header.nId[5] != 1)
	{
		throw runtime_error("This ELF file format is not supported. Only 32-bits LSB ordered ELFs are supported.");
	}

	nCount = m_Header.nProgHeaderCount;
	m_pProgram = new ELFPROGRAMHEADER[nCount];

	pS->Seek(m_Header.nProgHeaderStart, Framework::STREAM_SEEK_SET);
	for(i = 0; i < nCount; i++)
	{
		pS->Read(&m_pProgram[i], sizeof(ELFPROGRAMHEADER));
	}

	nCount = m_Header.nSectHeaderCount;
	m_pSection = new ELFSECTIONHEADER[nCount];

	pS->Seek(m_Header.nSectHeaderStart, Framework::STREAM_SEEK_SET);
	for(i = 0; i < nCount; i++)
	{
		pS->Read(&m_pSection[i], sizeof(ELFSECTIONHEADER));
	}
}

CELF::~CELF()
{
	delete [] m_pProgram;
	delete [] m_pSection;
	delete [] m_pData;
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
	ELFSECTIONHEADER* pSect;
	pSect = GetSection(nIndex);
	if(pSect == NULL) return NULL;
	return (const uint8*)m_pData + pSect->nOffset;
}

const void* CELF::FindSectionData(const char* sName)
{
	ELFSECTIONHEADER* pSect;
	pSect = FindSection(sName);
	if(pSect == NULL) return NULL;
	return (const uint8*)m_pData + pSect->nOffset;
}

ELFSECTIONHEADER* CELF::FindSection(const char* sName)
{
	unsigned int i;
	ELFSECTIONHEADER* pTemp;
	const char* sSectData;
	const char* sSectName;

	sSectData = (const char*)GetSectionData(m_Header.nSectHeaderStringTableIndex);
	if(sSectData == NULL) return NULL;
	for(i = 0; i < m_Header.nSectHeaderCount; i++)
	{
		pTemp = GetSection(i);
		sSectName = sSectData + pTemp->nStringTableIndex;
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
