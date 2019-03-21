#pragma once

#include "win32/MDIChild.h"
#include "DisAsm.h"
#include "../VirtualMachine.h"
#include "VirtualMachineStateView.h"

class CDisAsmWnd : public Framework::Win32::CMDIChild, public CVirtualMachineStateView
{
public:
	enum DISASM_TYPE
	{
		DISASM_STANDARD,
		DISASM_VU
	};

	CDisAsmWnd(HWND, CVirtualMachine&, CMIPS*, DISASM_TYPE);
	virtual ~CDisAsmWnd();

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

	CDisAsm* GetDisAsm() const;

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;
	long OnSysCommand(unsigned int, LPARAM) override;
	long OnSetFocus() override;

private:
	void RefreshLayout();
	CDisAsm* m_disAsm = nullptr;
};
