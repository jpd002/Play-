#pragma once

#include "win32/MDIChild.h"
#include "win32/Tab.h"
#include "VirtualMachineStateView.h"

class CMIPS;
class CRegViewPage;

class CRegViewWnd : public Framework::Win32::CMDIChild, public CVirtualMachineStateView
{
public:
	CRegViewWnd(HWND, CMIPS*);
	virtual ~CRegViewWnd();

	void HandleMachineStateChange() override;

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;
	long OnSysCommand(unsigned int, LPARAM) override;
	LRESULT OnNotify(WPARAM, NMHDR*) override;

private:
	enum
	{
		MAXTABS = 4,
	};

	void SelectTab(unsigned int);
	void UnselectTab(unsigned int);
	void RefreshLayout();

	CRegViewPage* m_regView[MAXTABS];
	CRegViewPage* m_current = nullptr;
	Framework::Win32::CTab m_tabs;
};
