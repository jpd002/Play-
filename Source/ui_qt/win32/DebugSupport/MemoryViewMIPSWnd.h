#pragma once

#include "win32/MDIChild.h"
#include "win32/Edit.h"
#include "MemoryViewMIPS.h"
#include "../VirtualMachine.h"
#include "VirtualMachineStateView.h"

class CMemoryViewMIPSWnd : public Framework::Win32::CMDIChild, public CVirtualMachineStateView
{
public:
	CMemoryViewMIPSWnd(HWND, CVirtualMachine&, CMIPS*);
	~CMemoryViewMIPSWnd();

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

	CMemoryViewMIPS* GetMemoryView() const;

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;
	long OnSysCommand(unsigned int, LPARAM) override;
	long OnSetFocus() override;

private:
	void RefreshLayout();
	void UpdateStatusBar();
	void OnMemoryViewSelectionChange(uint32);

	CMemoryViewMIPS* m_memoryView = nullptr;
	Framework::Win32::CEdit* m_addressEdit = nullptr;
};
