#ifndef _MEMORYVIEWMIPSWND_H_
#define _MEMORYVIEWMIPSWND_H_

#include "win32/MDIChild.h"
#include "MemoryViewMIPS.h"

class CMemoryViewMIPSWnd : public Framework::CMDIChild
{
public:
						CMemoryViewMIPSWnd(HWND, CMIPS*);
						~CMemoryViewMIPSWnd();

protected:
	long				OnSize(unsigned int, unsigned int, unsigned int);
	long				OnSysCommand(unsigned int, LPARAM);
	long				OnSetFocus();

private:
	void				RefreshLayout();
	CMemoryViewMIPS*	m_pMemoryView;

};

#endif
