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

class CElfFile : protected CElfFileContainer, public CELF
{
public:
	CElfFile(Framework::CStream&);
	virtual ~CElfFile();

private:
};
