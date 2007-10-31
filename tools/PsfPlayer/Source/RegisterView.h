#ifndef _REGISTERVIEW_H_
#define _REGISTERVIEW_H_

#include "DebugView.h"
#include "MIPS.h"

class CRegisterView : public CDebugView
{
public:
                CRegisterView(CMIPS&);
    virtual     ~CRegisterView();

    void        Update(WINDOW*, int, int);

private:
    CMIPS&      m_cpu;
};

#endif
