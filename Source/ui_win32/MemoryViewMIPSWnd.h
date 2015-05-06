#pragma once

#include "win32/MDIChild.h"
#include "win32/Edit.h"
#include "MemoryViewMIPS.h"
#include "../VirtualMachine.h"

class CMemoryViewMIPSWnd : public Framework::Win32::CMDIChild
{
public:
									CMemoryViewMIPSWnd(HWND, CVirtualMachine&, CMIPS*);
									~CMemoryViewMIPSWnd();

	CMemoryViewMIPS*				GetMemoryView() const;

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int) override;
	long							OnSysCommand(unsigned int, LPARAM) override;
	long							OnSetFocus() override;

private:
	void							RefreshLayout();
	void							UpdateStatusBar();
	void							OnMemoryViewSelectionChange(uint32);

	CMemoryViewMIPS*				m_memoryView = nullptr;
	Framework::Win32::CEdit*		m_addressEdit = nullptr;
};
