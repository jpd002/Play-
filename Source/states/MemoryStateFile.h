#pragma once

#include "zip/ZipFile.h"

class CMemoryStateFile : public Framework::CZipFile
{
public:
	CMemoryStateFile(const char*, const void*, size_t);
	virtual ~CMemoryStateFile() = default;

	void Write(Framework::CStream&) override;

private:
	const void* m_memory = nullptr;
	size_t m_size = 0;
};
