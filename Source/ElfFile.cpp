#include "ElfFile.h"

CElfFileContainer::CElfFileContainer(Framework::CStream& input)
{
	m_size = input.GetLength();
	m_content = new uint8[m_size];
	input.Read(m_content, m_size);
}

CElfFileContainer::~CElfFileContainer()
{
	delete[] m_content;
}

uint8* CElfFileContainer::GetFileContent() const
{
	return m_content;
}

uint64 CElfFileContainer::GetFileSize() const
{
	return m_size;
}
