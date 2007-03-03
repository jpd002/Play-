#ifndef _DISASMWND_H_
#define _DISASMWND_H_

#include "win32/MDIChild.h"
#include "DisAsm.h"

class CDisAsmWnd : public Framework::Win32::CMDIChild
{
public:
						CDisAsmWnd(HWND, CMIPS*);
						~CDisAsmWnd();
	void				Refresh();
	void				SetAddress(uint32);

protected:
	long				OnSize(unsigned int, unsigned int, unsigned int);
	long				OnSysCommand(unsigned int, LPARAM);
	long				OnSetFocus();
    long                OnCopy();

private:
	void				RefreshLayout();
	CDisAsm*			m_pDisAsm;
};

#endif