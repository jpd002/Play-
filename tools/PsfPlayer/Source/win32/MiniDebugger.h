#ifndef _MINIDEBUGGER_H_
#define _MINIDEBUGGER_H_

#include "win32/Window.h"
#include "win32/HorizontalSplitter.h"
#include "win32ui/DisAsm.h"
#include "../PsfVm.h"

class CMiniDebugger : public Framework::Win32::CWindow
{
public:
                                                CMiniDebugger(CPsfVm&);
    virtual                                     ~CMiniDebugger();
    void                                        Run();

protected:
    long                                        OnCommand(unsigned short, unsigned short, HWND);

private:
    void                                        CreateAccelerators();
    void                                        StepCPU();

    CPsfVm&                                     m_virtualMachine;
    Framework::Win32::CHorizontalSplitter*      m_splitter;
    CDisAsm*                                    m_disAsmView;
    HACCEL                                      m_acceleratorTable;
};

#endif
