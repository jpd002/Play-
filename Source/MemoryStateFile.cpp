#include "MemoryStateFile.h"

using namespace Framework;

CMemoryStateFile::CMemoryStateFile(const char* name, void* memory, size_t size) :
CZipFile(name),
m_memory(memory),
m_size(size)
{

}

CMemoryStateFile::~CMemoryStateFile()
{
    
}

void CMemoryStateFile::Write(CStream& stream)
{
    stream.Write(m_memory, m_size);
}
