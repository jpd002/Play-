#include "ElfViewEx.h"

using namespace Framework;

CElfViewEx::CElfViewEx(HWND parentWnd)
    : CELFView(parentWnd)
    , m_elf(NULL)
{
}

CElfViewEx::~CElfViewEx()
{
	SetELF(NULL);

	if(m_elf != NULL)
	{
		delete m_elf;
		m_elf = NULL;
	}
}

void CElfViewEx::LoadElf(Framework::CStream& stream)
{
	if(m_elf != NULL)
	{
		delete m_elf;
		m_elf = NULL;
	}

	m_elf = new CElfFile(stream);
	SetELF(m_elf);
}
