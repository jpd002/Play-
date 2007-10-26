#ifndef _ELFVIEWFRAME_H_
#define _ELFVIEWFRAME_H_

#include "win32/MDIFrame.h"
#include "ElfViewEx.h"

class CElfViewFrame : public Framework::Win32::CMDIFrame
{
public:
                CElfViewFrame(const char*);
    virtual     ~CElfViewFrame();

private:
    CElfViewEx* Open(const char*);
    CElfViewEx* m_elfView;
};

#endif
