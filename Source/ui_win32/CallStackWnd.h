#pragma once

#include "../BiosDebugInfoProvider.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"
#include "Types.h"
#include "win32/ListView.h"
#include "win32/MDIChild.h"
#include <boost/signals2.hpp>

class CCallStackWnd : public Framework::Win32::CMDIChild, public boost::signals2::trackable
{
public:
	typedef boost::signals2::signal<void(uint32)> OnFunctionDblClickSignal;

	CCallStackWnd(HWND, CVirtualMachine&, CMIPS*, CBiosDebugInfoProvider*);
	virtual ~CCallStackWnd();

	OnFunctionDblClickSignal OnFunctionDblClick;

protected:
	long    OnSize(unsigned int, unsigned int, unsigned int) override;
	long    OnSysCommand(unsigned int, LPARAM) override;
	LRESULT OnNotify(WPARAM, NMHDR*) override;

private:
	void RefreshLayout();
	void CreateColumns();
	void Update();
	void OnListDblClick();

	CVirtualMachine&             m_virtualMachine;
	CMIPS*                       m_context;
	Framework::Win32::CListView* m_list;
	CBiosDebugInfoProvider*      m_biosDebugInfoProvider;
};
