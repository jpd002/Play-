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
						CELF(Framework::CStream*);
						~CELF();
	ELFSECTIONHEADER*	GetSection(unsigned int);
	ELFSECTIONHEADER*	FindSection(const char*);
	const void*			GetSectionData(unsigned int);
	const void*			FindSectionData(const char*);
	ELFPROGRAMHEADER*	GetProgram(unsigned int);
	ELFHEADER			m_Header;
	char*				m_pData;
	int					m_nLenght;
private:
	ELFSECTIONHEADER*	m_pSection;
	ELFPROGRAMHEADER*	m_pProgram;
};

#endif
