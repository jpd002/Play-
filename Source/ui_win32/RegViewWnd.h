#pragma once

#include "win32/MDIChild.h"
#include "win32/Tab.h"
#include "../VirtualMachine.h"

class CMIPS;

class CRegViewWnd : public Framework::Win32::CMDIChild, public boost::signals2::trackable
{
public:
							CRegViewWnd(HWND, CVirtualMachine&, CMIPS*);
	virtual					~CRegViewWnd();

protected:
	long					OnSize(unsigned int, unsigned int, unsigned int) override;
	long					OnSysCommand(unsigned int, LPARAM) override;
	long					OnNotify(WPARAM, NMHDR*) override;

private:
	enum
	{
		MAXTABS			= 4,
	};

	void					SelectTab(unsigned int);
	void					UnselectTab(unsigned int);
	void					RefreshLayout();

	CWindow*				m_regView[MAXTABS];
	CWindow*				m_current = nullptr;
	Framework::Win32::CTab	m_tabs;
};
