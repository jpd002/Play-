#pragma once

#include <vector>
#include "Types.h"

#pragma pack(push, 1)

struct ELFSYMBOL32
{
	uint32 nName;
	uint32 nValue;
	uint32 nSize;
	uint8 nInfo;
	uint8 nOther;
	uint16 nSectionIndex;
};

struct ELFSYMBOL64
{
	uint32 nName;
	uint8 nInfo;
	uint8 nOther;
	uint16 nSectionIndex;
	uint64 nValue;
	uint64 nSize;
};

struct ELFHEADER32
{
	uint8 nId[16];
	uint16 nType;
	uint16 nCPU;
	uint32 nVersion;
	uint32 nEntryPoint;
	uint32 nProgHeaderStart;
	uint32 nSectHeaderStart;
	uint32 nFlags;
	uint16 nSize;
	uint16 nProgHeaderEntrySize;
	uint16 nProgHeaderCount;
	uint16 nSectHeaderEntrySize;
	uint16 nSectHeaderCount;
	uint16 nSectHeaderStringTableIndex;
};

struct ELFHEADER64
{
	uint8 nId[16];
	uint16 nType;
	uint16 nCPU;
	uint32 nVersion;
	uint64 nEntryPoint;
	uint64 nProgHeaderStart;
	uint64 nSectHeaderStart;
	uint32 nFlags;
	uint16 nSize;
	uint16 nProgHeaderEntrySize;
	uint16 nProgHeaderCount;
	uint16 nSectHeaderEntrySize;
	uint16 nSectHeaderCount;
	uint16 nSectHeaderStringTableIndex;
};

struct ELFSECTIONHEADER32
{
	uint32 nStringTableIndex;
	uint32 nType;
	uint32 nFlags;
	uint32 nStart;
	uint32 nOffset;
	uint32 nSize;
	uint32 nIndex;
	uint32 nInfo;
	uint32 nAlignment;
	uint32 nOther;
};

struct ELFSECTIONHEADER64
{
	uint32 nStringTableIndex;
	uint32 nType;
	uint64 nFlags;
	uint64 nStart;
	uint64 nOffset;
	uint64 nSize;
	uint32 nIndex;
	uint32 nInfo;
	uint64 nAlignment;
	uint64 nOther;
};

struct ELFPROGRAMHEADER32
{
	uint32 nType;
	uint32 nOffset;
	uint32 nVAddress;
	uint32 nPAddress;
	uint32 nFileSize;
	uint32 nMemorySize;
	uint32 nFlags;
	uint32 nAlignment;
};

struct ELFPROGRAMHEADER64
{
	uint32 nType;
	uint32 nFlags;
	uint64 nOffset;
	uint64 nVAddress;
	uint64 nPAddress;
	uint64 nFileSize;
	uint64 nMemorySize;
	uint64 nAlignment;
};

#pragma pack(pop)

class CELF
{
public:
	typedef ELFHEADER32 HEADER;
	typedef ELFPROGRAMHEADER32 PROGRAMHEADER;
	typedef ELFSECTIONHEADER32 SECTIONHEADER;
	typedef ELFSYMBOL32 ELFSYMBOL;

	enum ELFHEADERID
	{
		EI_CLASS = 4,
		EI_DATA = 5,
	};

	enum ELFCLASS
	{
		ELFCLASS32 = 1,
		ELFCLASS64 = 2,
	};

	enum ELFDATA
	{
		ELFDATA2LSB = 1,
		ELFDATA2MSB = 2,
	};

	enum EXECUTABLE_TYPE
	{
		ET_NONE = 0,
		ET_REL = 1,
		ET_EXEC = 2,
		ET_DYN = 3,
		ET_CORE = 4,
	};

	enum MACHINE_TYPE
	{
		EM_NONE = 0,
		EM_M32 = 1,
		EM_SPARC = 2,
		EM_386 = 3,
		EM_68K = 4,
		EM_88K = 5,
		EM_860 = 7,
		EM_MIPS = 8,
		EM_PPC64 = 21,
		EM_ARM = 40,
	};

	enum EXECUTABLE_VERSION
	{
		EV_NONE = 0,
		EV_CURRENT = 1,
	};

	enum SECTION_HEADER_TYPE
	{
		SHT_NULL = 0,
		SHT_PROGBITS = 1,
		SHT_SYMTAB = 2,
		SHT_STRTAB = 3,
		SHT_HASH = 5,
		SHT_DYNAMIC = 6,
		SHT_NOTE = 7,
		SHT_NOBITS = 8,
		SHT_REL = 9,
		SHT_DYNSYM = 11,
	};

	enum SECTION_HEADER_FLAG
	{
		SHF_WRITE = 0x0001,
		SHF_ALLOC = 0x0002,
		SHF_EXECINSTR = 0x0004,
	};

	enum PROGRAM_HEADER_TYPE
	{
		PT_NULL = 0,
		PT_LOAD = 1,
		PT_DYNAMIC = 2,
		PT_INTERP = 3,
		PT_NOTE = 4,
		PT_SHLIB = 5,
		PT_PHDR = 6,
	};

	enum PROGRAM_HEADER_FLAG
	{
		PF_X = 0x01,
		PF_W = 0x02,
		PF_R = 0x04,
	};

	enum DYNAMIC_INFO_TYPE
	{
		DT_NONE = 0,
		DT_NEEDED = 1,
		DT_PLTRELSZ = 2,
		DT_PLTGOT = 3,
		DT_HASH = 4,
		DT_STRTAB = 5,
		DT_SYMTAB = 6,
		DT_SONAME = 14,
		DT_SYMBOLIC = 16,
	};

	enum MIPS_RELOCATION_TYPE
	{
		R_MIPS_32 = 2,
		R_MIPS_26 = 4,
		R_MIPS_HI16 = 5,
		R_MIPS_LO16 = 6,
		R_MIPS_GPREL16 = 7,
	};

	CELF(uint8*);
	CELF(const CELF&) = delete;
	virtual ~CELF() = default;

	CELF& operator=(const CELF&) = delete;

	uint8* GetContent() const;
	const HEADER& GetHeader() const;

	SECTIONHEADER* GetSection(unsigned int);
	const void* GetSectionData(unsigned int);
	const char* GetSectionName(unsigned int);

	SECTIONHEADER* FindSection(const char*);
	unsigned int FindSectionIndex(const char*);
	const void* FindSectionData(const char*);

	PROGRAMHEADER* GetProgram(unsigned int);

private:
	HEADER m_header;
	uint8* m_content = nullptr;

	std::vector<SECTIONHEADER> m_sections;
	std::vector<PROGRAMHEADER> m_programs;
};
