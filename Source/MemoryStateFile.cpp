#include "MemoryStateFile.h"

CMemoryStateFile::CMemoryStateFile(const char* name, const void* memory, size_t size) 
: CZipFile(name)
, m_memory(memory)
, m_size(size)
{

}

CMemoryStateFile::~CMemoryStateFile()
{

}

void CMemoryStateFile::Write(Framework::CStream& stream)
{
	stream.Write(m_memory, m_size);
}
