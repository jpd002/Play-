#ifndef _MEMORYVIEWMIPSWND_H_
#define _MEMORYVIEWMIPSWND_H_

#include "win32/MDIChild.h"
#include "win32/StatusBar.h"
#include "MemoryViewMIPS.h"

class CMemoryViewMIPSWnd : public Framework::CMDIChild
{
public:
						            CMemoryViewMIPSWnd(HWND, CMIPS*);
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
