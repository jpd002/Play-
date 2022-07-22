#pragma once

#include "ELF.h"
#include "Stream.h"

class CElfFileContainer
{
public:
	CElfFileContainer(Framework::CStream&);
	virtual ~CElfFileContainer();

	uint8* GetFileContent() const;

private:
	uint8* m_content;
};

template <typename ElfType>
class CElfFile : protected CElfFileContainer, public ElfType
{
public:
	CElfFile(Framework::CStream& stream)
	    : CElfFileContainer(stream)
	    , ElfType(GetFileContent())
	{
	}

	virtual ~CElfFile() = default;
};

typedef CElfFile<CELF32> CElf32File;
typedef CElfFile<CELF64> CElf64File;
