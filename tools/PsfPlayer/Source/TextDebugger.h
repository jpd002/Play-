#ifndef _TEXTDEBUGGER_H_
#define _TEXTDEBUGGER_H_

#include <curses.h>
#include <string>
#include "MIPS.h"
#include "DisAsmView.h"
#include "MemoryView.h"

class CTextDebugger
{
public:
                    CTextDebugger(CMIPS&);
    virtual         ~CTextDebugger();

    void            Run();

private:
    void            ExecuteCommand(const std::string&);

    WINDOW*         m_window;
    int             m_width;
    int             m_height;
    CMIPS&          m_cpu;
    CDebugView*     m_currentView;
    CDisAsmView     m_disAsmView;
    CMemoryView     m_memoryView;
};

#endif
