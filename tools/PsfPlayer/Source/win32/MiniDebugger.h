#ifndef _MINIDEBUGGER_H_
#define _MINIDEBUGGER_H_

#include "win32/Window.h"
#include "win32/HorizontalSplitter.h"
#include "win32ui/DisAsm.h"
#include "win32ui/RegViewGeneral.h"
#include "FunctionsView.h"
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
    void                                        OnFunctionDblClick(uint32);

    CPsfVm&                                     m_virtualMachine;
    Framework::Win32::CHorizontalSplitter*      m_splitter;
    CDisAsm*                                    m_disAsmView;
    CRegViewGeneral*                            m_registerView;
    CFunctionsView*                             m_functionsView;
    HACCEL                                      m_acceleratorTable;
};

#endif
