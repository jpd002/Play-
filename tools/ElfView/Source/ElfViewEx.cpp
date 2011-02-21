#include "ElfViewEx.h"

using namespace Framework;

CElfViewEx::CElfViewEx(HWND parentWnd, CStream& stream) :
CELFView(parentWnd),
m_elf(stream)
{
    SetELF(&m_elf);
}

CElfViewEx::~CElfViewEx()
{
    SetELF(NULL);
}
