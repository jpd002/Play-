#ifndef _ELF_H_
#define _ELF_H_

#include "Stream.h"
#include "Types.h"

#pragma pack(push, 1)

struct ELFSYMBOL
{
	uint32		nName;
	uint32		nValue;
	uint32		nSize;
	uint8		nInfo;
	uint8		nOther;
	uint16		nSectionIndex;
};

struct ELFHEADER
{
	uint8		nId[16];
	uint16		nType;
	uint16		nCPU;
	uint32		nVersion;
	uint32		nEntryPoint;
	uint32		nProgHeaderStart;
	uint32		nSectHeaderStart;
	uint32		nFlags;
	uint16		nSize;
	uint16		nProgHeaderEntrySize;
	uint16		nProgHeaderCount;
	uint16		nSectHeaderEntrySize;
	uint16		nSectHeaderCount;
	uint16		nSectHeaderStringTableIndex;
};

struct ELFSECTIONHEADER
{
	uint32		nStringTableIndex;
	uint32		nType;
	uint32		nFlags;
	uint32		nStart;
	uint32		nOffset;
	uint32		nSize;
	uint32		nIndex;
	uint32		nInfo;
	uint32		nAlignment;
	uint32		nOther;
};

struct ELFPROGRAMHEADER
{
	uint32		nType;
	uint32		nOffset;
	uint32		nVAddress;
	uint32		nPAddress;
	uint32		nFileSize;
	uint32		nMemorySize;
	uint32		nFlags;
	uint32		nAlignment;
};

#pragma pack(pop)

class CELF
{
public:
	enum EXECUTABLE_TYPE
	{
		ET_NONE			= 0,
		ET_REL			= 1,
		ET_EXEC			= 2,
		ET_DYN			= 3,
		ET_CORE			= 4,
	};

	enum MACHINE_TYPE
	{
		EM_NONE			= 0,
		EM_M32			= 1,
		EM_SPARC		= 2,
		EM_386			= 3,
		EM_68K			= 4,
		EM_88K			= 5,
		EM_860			= 7,
		EM_MIPS			= 8,
		EM_ARM			= 40,
	};

	enum EXECUTABLE_VERSION
	{
		EV_NONE			= 0,
		EV_CURRENT		= 1,
	};

	enum SECTION_HEADER_TYPE
	{
		SHT_NULL		= 0,
		SHT_PROGBITS	= 1,
		SHT_SYMTAB		= 2,
		SHT_STRTAB		= 3,
		SHT_HASH		= 5,
		SHT_DYNAMIC		= 6,
		SHT_NOTE		= 7,
		SHT_NOBITS		= 8,
		SHT_REL			= 9,
		SHT_DYNSYM		= 11,
	};

	enum PROGRAM_HEADER_TYPE
	{
		PT_NULL			= 0,
		PT_LOAD			= 1,
		PT_DYNAMIC		= 2,
		PT_INTERP		= 3,
		PT_NOTE			= 4,
		PT_SHLIB		= 5,
		PT_PHDR			= 6,
	};

	enum PROGRAM_HEADER_FLAG
	{
		PF_X			= 0x01,
		PF_W			= 0x02,
		PF_R			= 0x04,
	};

	enum DYNAMIC_INFO_TYPE
	{
		DT_NONE			= 0,
		DT_NEEDED		= 1,
		DT_PLTRELSZ		= 2,
		DT_PLTGOT		= 3,
		DT_HASH			= 4,
		DT_STRTAB		= 5,
		DT_SYMTAB		= 6,
		DT_SONAME		= 14,
		DT_SYMBOLIC		= 16,
	};

	enum MIPS_RELOCATION_TYPE
	{
		R_MIPS_32 = 2,
		R_MIPS_26 = 4,
		R_MIPS_HI16 = 5,
		R_MIPS_LO16 = 6,
	};

						CELF(uint8*);
	virtual				~CELF();

	uint8*				GetContent() const;
	const ELFHEADER&	GetHeader() const;
	ELFSECTIONHEADER*	GetSection(unsigned int);
	ELFSECTIONHEADER*	FindSection(const char*);
	const void*			GetSectionData(unsigned int);
	const void*			FindSectionData(const char*);
	ELFPROGRAMHEADER*	GetProgram(unsigned int);

private:
	ELFHEADER			m_Header;
	uint8*				m_content;

	ELFSECTIONHEADER*	m_pSection;
	ELFPROGRAMHEADER*	m_pProgram;
};

#endif
