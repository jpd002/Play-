#ifndef _MEMORYVIEW_H_
#define _MEMORYVIEW_H_

#include "DebugView.h"
#include "MIPS.h"

class CMemoryView : public CDebugView
{
public:
                    CMemoryView(CMIPS&);
    virtual         ~CMemoryView();

    virtual void    Update(WINDOW*, int, int);
    void            SetViewAddress(uint32);

private:
    CMIPS&          m_cpu;
    uint32          m_viewAddress;
};

#endif
