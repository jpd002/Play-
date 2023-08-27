#pragma once

#include "ELF.h"
#include "Stream.h"

class CElfFileContainer
{
public:
	CElfFileContainer(Framework::CStream&);
	virtual ~CElfFileContainer();

	uint8* GetFileContent() const;
	uint64 GetFileSize() const;

private:
	uint8* m_content = nullptr;
	uint64 m_size = 0;
};

template <typename ElfType>
class CElfFile : protected CElfFileContainer, public ElfType
{
public:
	CElfFile(Framework::CStream& stream)
	    : CElfFileContainer(stream)
	    , ElfType(GetFileContent(), GetFileSize())
	{
	}

	virtual ~CElfFile() = default;
};

typedef CElfFile<CELF32> CElf32File;
typedef CElfFile<CELF64> CElf64File;
