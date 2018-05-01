#include <assert.h>
#include <string.h>
#include <stdexcept>
#include "ELF.h"
#include "PtrStream.h"

CELF::CELF(uint8* content)
    : m_content(content)
{
	Framework::CPtrStream stream(m_content, -1);

	stream.Read(&m_Header, sizeof(ELFHEADER));

	if(m_Header.nId[0] != 0x7F || m_Header.nId[1] != 'E' || m_Header.nId[2] != 'L' || m_Header.nId[3] != 'F')
	{
		throw std::runtime_error("This file isn't a valid ELF file.");
	}

	if(m_Header.nId[4] != 1 || m_Header.nId[5] != 1)
	{
		throw std::runtime_error("This ELF file format is not supported. Only 32-bits LSB ordered ELFs are supported.");
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
	delete[] m_pProgram;
	delete[] m_pSection;
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
		return nullptr;
	}
	return &m_pSection[nIndex];
}

const void* CELF::GetSectionData(unsigned int nIndex)
{
	auto section = GetSection(nIndex);
	if(section == nullptr) return nullptr;
	return m_content + section->nOffset;
}

const char* CELF::GetSectionName(unsigned int sectionIndex)
{
	auto stringTableData = reinterpret_cast<const char*>(GetSectionData(m_Header.nSectHeaderStringTableIndex));
	if(stringTableData == nullptr) return nullptr;
	auto sectionHeader = GetSection(sectionIndex);
	if(sectionHeader == nullptr) return nullptr;
	return stringTableData + sectionHeader->nStringTableIndex;
}

ELFSECTIONHEADER* CELF::FindSection(const char* requestedSectionName)
{
	auto sectionIndex = FindSectionIndex(requestedSectionName);
	if(sectionIndex == 0) return nullptr;
	return GetSection(sectionIndex);
}

unsigned int CELF::FindSectionIndex(const char* requestedSectionName)
{
	auto stringTableData = reinterpret_cast<const char*>(GetSectionData(m_Header.nSectHeaderStringTableIndex));
	if(stringTableData == nullptr) return 0;
	for(unsigned int i = 0; i < m_Header.nSectHeaderCount; i++)
	{
		auto sectionHeader = GetSection(i);
		auto sectionName = stringTableData + sectionHeader->nStringTableIndex;
		if(!strcmp(sectionName, requestedSectionName))
		{
			return i;
		}
	}
	return 0;
}

const void* CELF::FindSectionData(const char* requestedSectionName)
{
	auto section = FindSection(requestedSectionName);
	if(section == nullptr) return nullptr;
	return m_content + section->nOffset;
}

ELFPROGRAMHEADER* CELF::GetProgram(unsigned int nIndex)
{
	if(nIndex >= m_Header.nProgHeaderCount)
	{
		return nullptr;
	}
	return &m_pProgram[nIndex];
}
