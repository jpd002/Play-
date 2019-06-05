#pragma once

#include <boost/signals2.hpp>
#include "win32/MDIChild.h"
#include "win32/ListView.h"
#include "Types.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CBiosDebugInfoProvider;

class CCallStackWnd : public Framework::Win32::CMDIChild, public CVirtualMachineStateView
{
public:
	typedef boost::signals2::signal<void(uint32)> OnFunctionDblClickSignal;

	CCallStackWnd(HWND, CMIPS*, CBiosDebugInfoProvider*);
	virtual ~CCallStackWnd();

	void HandleMachineStateChange() override;

	OnFunctionDblClickSignal OnFunctionDblClick;

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;
	long OnSysCommand(unsigned int, LPARAM) override;
	LRESULT OnNotify(WPARAM, NMHDR*) override;

private:
	void RefreshLayout();
	void CreateColumns();
	void Update();
	void OnListDblClick();

	CMIPS* m_context;
	Framework::Win32::CListView* m_list;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider;
};
