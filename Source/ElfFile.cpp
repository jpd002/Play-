#include "ElfFile.h"

CElfFileContainer::CElfFileContainer(Framework::CStream& input)
{
	uint32 size = static_cast<uint32>(input.GetLength());
	m_content = new uint8[size];
	input.Read(m_content, size);
}

CElfFileContainer::~CElfFileContainer()
{
	delete[] m_content;
}

uint8* CElfFileContainer::GetFileContent() const
{
	return m_content;
}
