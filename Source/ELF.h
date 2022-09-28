#pragma once

#include <vector>
#include <stdexcept>
#include "Types.h"
#include "ElfDefs.h"
#include "PtrStream.h"
#include "EndianUtils.h"

struct ELFTRAITS32
{
	typedef ELF::ELFHEADER32 ELFHEADER;
	typedef ELF::ELFPROGRAMHEADER32 ELFPROGRAMHEADER;
	typedef ELF::ELFSECTIONHEADER32 ELFSECTIONHEADER;
	typedef ELF::ELFSYMBOL32 ELFSYMBOL;
	enum
	{
		HEADER_ID_CLASS = ELF::ELFCLASS32
	};
};

struct ELFTRAITS64
{
	typedef ELF::ELFHEADER64 ELFHEADER;
	typedef ELF::ELFPROGRAMHEADER64 ELFPROGRAMHEADER;
	typedef ELF::ELFSECTIONHEADER64 ELFSECTIONHEADER;
	typedef ELF::ELFSYMBOL64 ELFSYMBOL;
	enum
	{
		HEADER_ID_CLASS = ELF::ELFCLASS64
	};
};

template <typename ElfTraits>
class CELF
{
public:
	typedef typename ElfTraits::ELFHEADER HEADER;
	typedef typename ElfTraits::ELFPROGRAMHEADER PROGRAMHEADER;
	typedef typename ElfTraits::ELFSECTIONHEADER SECTIONHEADER;
	typedef typename ElfTraits::ELFSYMBOL SYMBOL;

	CELF(uint8* content)
	    : m_content(content)
	{
		Framework::CPtrStream stream(m_content, -1);

		stream.Read(&m_header, sizeof(m_header));

		if(m_header.nId[0] != 0x7F || m_header.nId[1] != 'E' || m_header.nId[2] != 'L' || m_header.nId[3] != 'F')
		{
			throw std::runtime_error("This file isn't a valid ELF file.");
		}

		if(m_header.nId[ELF::EI_CLASS] != ElfTraits::HEADER_ID_CLASS)
		{
			throw std::runtime_error("Failed to load ELF file: wrong bitness.");
		}

		if(m_header.nId[ELF::EI_DATA] == ELF::ELFDATA2MSB)
		{
			Framework::CEndian::FromMSBF(m_header.nType);
			Framework::CEndian::FromMSBF(m_header.nCPU);
			Framework::CEndian::FromMSBF(m_header.nVersion);
			Framework::CEndian::FromMSBF(m_header.nEntryPoint);
			Framework::CEndian::FromMSBF(m_header.nProgHeaderStart);
			Framework::CEndian::FromMSBF(m_header.nSectHeaderStart);
			Framework::CEndian::FromMSBF(m_header.nFlags);
			Framework::CEndian::FromMSBF(m_header.nSize);
			Framework::CEndian::FromMSBF(m_header.nProgHeaderEntrySize);
			Framework::CEndian::FromMSBF(m_header.nProgHeaderCount);
			Framework::CEndian::FromMSBF(m_header.nSectHeaderEntrySize);
			Framework::CEndian::FromMSBF(m_header.nSectHeaderCount);
			Framework::CEndian::FromMSBF(m_header.nSectHeaderStringTableIndex);
		}

		ReadProgramHeaders(stream);
		ReadSectionHeaders(stream);
	}

	CELF(const CELF&) = delete;
	virtual ~CELF() = default;

	CELF& operator=(const CELF&) = delete;

	uint8* GetContent() const
	{
		return m_content;
	}

	const HEADER& GetHeader() const
	{
		return m_header;
	}

	uint64 GetSectionHeaderCount() const
	{
		return m_sections.size();
	}

	SECTIONHEADER* GetSection(unsigned int index)
	{
		if(index >= m_sections.size())
		{
			return nullptr;
		}
		return &m_sections[index];
	}

	const void* GetSectionData(unsigned int index)
	{
		auto section = GetSection(index);
		if(section == nullptr) return nullptr;
		return m_content + section->nOffset;
	}

	const char* GetSectionName(unsigned int sectionIndex)
	{
		auto stringTableData = reinterpret_cast<const char*>(GetSectionData(m_header.nSectHeaderStringTableIndex));
		if(stringTableData == nullptr) return nullptr;
		auto sectionHeader = GetSection(sectionIndex);
		if(sectionHeader == nullptr) return nullptr;
		return stringTableData + sectionHeader->nStringTableIndex;
	}

	SECTIONHEADER* FindSection(const char* requestedSectionName)
	{
		auto sectionIndex = FindSectionIndex(requestedSectionName);
		if(sectionIndex == 0) return nullptr;
		return GetSection(sectionIndex);
	}

	unsigned int FindSectionIndex(const char* requestedSectionName)
	{
		auto stringTableData = reinterpret_cast<const char*>(GetSectionData(m_header.nSectHeaderStringTableIndex));
		if(stringTableData == nullptr) return 0;
		for(unsigned int i = 0; i < GetSectionHeaderCount(); i++)
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

	const void* FindSectionData(const char* requestedSectionName)
	{
		auto section = FindSection(requestedSectionName);
		if(section == nullptr) return nullptr;
		return m_content + section->nOffset;
	}

	PROGRAMHEADER* GetProgram(unsigned int index)
	{
		if(index >= m_header.nProgHeaderCount)
		{
			return nullptr;
		}
		return &m_programs[index];
	}

private:
	void ReadSectionHeaders(Framework::CStream& stream)
	{
		//Fetch extended section header count if needed
		decltype(SECTIONHEADER::nSize) sectionHeaderCount = 0;
		if((m_header.nSectHeaderCount == 0) && (m_header.nSectHeaderStart != 0))
		{
			SECTIONHEADER zeroSectionHeader = {};
			stream.Seek(m_header.nSectHeaderStart, Framework::STREAM_SEEK_SET);
			stream.Read(&zeroSectionHeader, sizeof(zeroSectionHeader));
			if(m_header.nId[ELF::EI_DATA] == ELF::ELFDATA2MSB)
			{
				Framework::CEndian::FromMSBF(zeroSectionHeader.nSize);
			}
			assert(zeroSectionHeader.nSize != 0);
			sectionHeaderCount = zeroSectionHeader.nSize;
		}
		else
		{
			sectionHeaderCount = m_header.nSectHeaderCount;
		}

		m_sections.resize(sectionHeaderCount);
		stream.Seek(m_header.nSectHeaderStart, Framework::STREAM_SEEK_SET);
		for(auto& section : m_sections)
		{
			auto readAmount = stream.Read(&section, sizeof(section));
			assert(readAmount == sizeof(section));
			if(m_header.nId[ELF::EI_DATA] == ELF::ELFDATA2MSB)
			{
				Framework::CEndian::FromMSBF(section.nStringTableIndex);
				Framework::CEndian::FromMSBF(section.nType);
				Framework::CEndian::FromMSBF(section.nFlags);
				Framework::CEndian::FromMSBF(section.nStart);
				Framework::CEndian::FromMSBF(section.nOffset);
				Framework::CEndian::FromMSBF(section.nSize);
				Framework::CEndian::FromMSBF(section.nIndex);
				Framework::CEndian::FromMSBF(section.nInfo);
				Framework::CEndian::FromMSBF(section.nAlignment);
				Framework::CEndian::FromMSBF(section.nOther);
			}
		}
	}

	void ReadProgramHeaders(Framework::CStream& stream)
	{
		m_programs.resize(m_header.nProgHeaderCount);
		stream.Seek(m_header.nProgHeaderStart, Framework::STREAM_SEEK_SET);
		for(auto& program : m_programs)
		{
			stream.Read(&program, sizeof(program));
			if(m_header.nId[ELF::EI_DATA] == ELF::ELFDATA2MSB)
			{
				Framework::CEndian::FromMSBF(program.nType);
				Framework::CEndian::FromMSBF(program.nFlags);
				Framework::CEndian::FromMSBF(program.nOffset);
				Framework::CEndian::FromMSBF(program.nVAddress);
				Framework::CEndian::FromMSBF(program.nPAddress);
				Framework::CEndian::FromMSBF(program.nFileSize);
				Framework::CEndian::FromMSBF(program.nMemorySize);
				Framework::CEndian::FromMSBF(program.nAlignment);
			}
		}
	}

	HEADER m_header;
	uint8* m_content = nullptr;

	std::vector<SECTIONHEADER> m_sections;
	std::vector<PROGRAMHEADER> m_programs;
};

typedef CELF<ELFTRAITS32> CELF32;
typedef CELF<ELFTRAITS64> CELF64;
