#ifndef _ELFVIEWEX_H_
#define _ELFVIEWEX_H_

#include "ELFFile.h"
#include "win32ui\ELFView.h"

class CElfViewEx : public CELFView
{
public:
                CElfViewEx(HWND, Framework::CStream&);
    virtual     ~CElfViewEx();

protected:

private:
    CElfFile       m_elf;
};

#endif
