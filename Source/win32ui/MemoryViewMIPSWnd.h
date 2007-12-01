#ifndef _MEMORYVIEWMIPSWND_H_
#define _MEMORYVIEWMIPSWND_H_

#include "win32/MDIChild.h"
#include "win32/StatusBar.h"
#include "MemoryViewMIPS.h"
#include "../VirtualMachine.h"

class CMemoryViewMIPSWnd : public Framework::Win32::CMDIChild
{
public:
						            CMemoryViewMIPSWnd(HWND, CVirtualMachine&, CMIPS*);
						            ~CMemoryViewMIPSWnd();

protected:
	long				            OnSize(unsigned int, unsigned int, unsigned int);
	long				            OnSysCommand(unsigned int, LPARAM);
	long				            OnSetFocus();

private:
	void				            RefreshLayout();
    void                            UpdateStatusBar();
    void                            OnMemoryViewSelectionChange(uint32);

	CMemoryViewMIPS*	            m_pMemoryView;
    Framework::Win32::CStatusBar*   m_pStatusBar;

};

#endif
