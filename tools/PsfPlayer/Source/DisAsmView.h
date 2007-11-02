#ifndef _DISASMVIEW_H_
#define _DISASMVIEW_H_

#include "DebugView.h"
#include "MIPS.h"

class CDisAsmView : public CDebugView
{
public:
                    CDisAsmView(CMIPS&);
    virtual         ~CDisAsmView();
    virtual void    Update(WINDOW*, int, int);

    void            SetViewAddress(uint32);
    void            EnsureVisible(uint32, WINDOW*);

private:
    CMIPS&          m_cpu;
    uint32          m_viewAddress;
};

#endif
