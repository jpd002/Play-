#pragma once

#include "win32/MDIChild.h"
#include "DisAsm.h"
#include "../VirtualMachine.h"

class CDisAsmWnd : public Framework::Win32::CMDIChild
{
public:
	enum DISASM_TYPE
	{
		DISASM_STANDARD,
		DISASM_VU
	};

						CDisAsmWnd(HWND, CVirtualMachine&, CMIPS*, DISASM_TYPE);
						~CDisAsmWnd();
	void				Refresh();

	void				SetAddress(uint32);
	void				SetCenterAtAddress(uint32);
	void				SetSelectedAddress(uint32);

protected:
	long				OnSize(unsigned int, unsigned int, unsigned int) override;
	long				OnSysCommand(unsigned int, LPARAM) override;
	long				OnSetFocus() override;

private:
	void				RefreshLayout();
	CDisAsm*			m_disAsm;
};
