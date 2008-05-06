#ifndef _MINIDEBUGGER_H_
#define _MINIDEBUGGER_H_

#include "win32/Window.h"
#include "win32/HorizontalSplitter.h"
#include "win32/VerticalSplitter.h"
#include "win32ui/DisAsm.h"
#include "win32ui/RegViewGeneral.h"
#include "win32ui/MemoryViewMIPS.h"
#include "FunctionsView.h"
#include "PsxVm.h"

class CMiniDebugger : public Framework::Win32::CWindow
{
public:
                                                CMiniDebugger(CPsxVm&);
    virtual                                     ~CMiniDebugger();
    void                                        Run();

protected:
    long                                        OnCommand(unsigned short, unsigned short, HWND);

private:
    void                                        CreateAccelerators();
    void                                        StepCPU();
    void                                        OnFunctionDblClick(uint32);

    CPsxVm&                                     m_virtualMachine;
    Framework::Win32::CHorizontalSplitter*      m_subSplitter;
    Framework::Win32::CVerticalSplitter*        m_mainSplitter;
    CDisAsm*                                    m_disAsmView;
    CRegViewGeneral*                            m_registerView;
    CMemoryViewMIPS*                            m_memoryView;
    CFunctionsView*                             m_functionsView;
    HACCEL                                      m_acceleratorTable;
};

#endif
