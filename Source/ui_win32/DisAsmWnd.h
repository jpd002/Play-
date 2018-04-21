#pragma once

#include "../VirtualMachine.h"
#include "DisAsm.h"
#include "win32/MDIChild.h"

class CDisAsmWnd : public Framework::Win32::CMDIChild
{
public:
	enum DISASM_TYPE
	{
		DISASM_STANDARD,
		DISASM_VU
	};

	CDisAsmWnd(HWND, CVirtualMachine&, CMIPS*, DISASM_TYPE);
	virtual ~CDisAsmWnd();

	void     Refresh();
	CDisAsm* GetDisAsm() const;

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;
	long OnSysCommand(unsigned int, LPARAM) override;
	long OnSetFocus() override;

private:
	void     RefreshLayout();
	CDisAsm* m_disAsm = nullptr;
};
