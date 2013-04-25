#pragma once

#include "zip/ZipFile.h"

class CMemoryStateFile : public Framework::CZipFile
{
public:
					CMemoryStateFile(const char*, const void*, size_t);
	virtual			~CMemoryStateFile();

	virtual void	Write(Framework::CStream&);

private:
	const void*		m_memory;
	size_t			m_size;
};
