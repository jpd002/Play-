#include "ELF.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include "PtrStream.h"
#include "Endian.h"

CELF::CELF(uint8* content)
    : m_content(content)
{
	Framework::CPtrStream stream(m_content, -1);

	stream.Read(&m_header, sizeof(m_header));

	if(m_header.nId[0] != 0x7F || m_header.nId[1] != 'E' || m_header.nId[2] != 'L' || m_header.nId[3] != 'F')
	{
		throw std::runtime_error("This file isn't a valid ELF file.");
	}

	if((m_header.nId[EI_CLASS] != ELFCLASS32) || (m_header.nId[EI_DATA] != ELFDATA2LSB))
	{
		throw std::runtime_error("This ELF file format is not supported. Only 32-bits LSB ordered ELFs are supported.");
	}

	{
		m_programs.resize(m_header.nProgHeaderCount);
		stream.Seek(m_header.nProgHeaderStart, Framework::STREAM_SEEK_SET);
		for(auto& program : m_programs)
		{
			stream.Read(&program, sizeof(program));
		}
	}

	{
		m_sections.resize(m_header.nSectHeaderCount);
		stream.Seek(m_header.nSectHeaderStart, Framework::STREAM_SEEK_SET);
		for(auto& section : m_sections)
		{
			stream.Read(&section, sizeof(section));
		}
	}
}

uint8* CELF::GetContent() const
{
	return m_content;
}

const CELF::HEADER& CELF::GetHeader() const
{
	return m_header;
}

CELF::SECTIONHEADER* CELF::GetSection(unsigned int nIndex)
{
	if(nIndex >= m_header.nSectHeaderCount)
	{
		return nullptr;
	}
	return &m_sections[nIndex];
}

const void* CELF::GetSectionData(unsigned int nIndex)
{
	auto section = GetSection(nIndex);
	if(section == nullptr) return nullptr;
	return m_content + section->nOffset;
}

const char* CELF::GetSectionName(unsigned int sectionIndex)
{
	auto stringTableData = reinterpret_cast<const char*>(GetSectionData(m_header.nSectHeaderStringTableIndex));
	if(stringTableData == nullptr) return nullptr;
	auto sectionHeader = GetSection(sectionIndex);
	if(sectionHeader == nullptr) return nullptr;
	return stringTableData + sectionHeader->nStringTableIndex;
}

CELF::SECTIONHEADER* CELF::FindSection(const char* requestedSectionName)
{
	auto sectionIndex = FindSectionIndex(requestedSectionName);
	if(sectionIndex == 0) return nullptr;
	return GetSection(sectionIndex);
}

unsigned int CELF::FindSectionIndex(const char* requestedSectionName)
{
	auto stringTableData = reinterpret_cast<const char*>(GetSectionData(m_header.nSectHeaderStringTableIndex));
	if(stringTableData == nullptr) return 0;
	for(unsigned int i = 0; i < m_header.nSectHeaderCount; i++)
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

CELF::PROGRAMHEADER* CELF::GetProgram(unsigned int nIndex)
{
	if(nIndex >= m_header.nProgHeaderCount)
	{
		return nullptr;
	}
	return &m_programs[nIndex];
}
