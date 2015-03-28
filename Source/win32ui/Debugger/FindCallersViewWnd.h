#pragma once

#include <boost/signals2.hpp>
#include "win32/MDIChild.h"
#include "win32/ListBox.h"
#include "../../MIPS.h"

class CFindCallersViewWnd : public Framework::Win32::CMDIChild
{
public:
	typedef boost::signals2::signal<void (uint32)> AddressSelectedEvent;

									CFindCallersViewWnd(HWND);
	virtual							~CFindCallersViewWnd();

	void							FindCallers(CMIPS*, uint32);

	AddressSelectedEvent			AddressSelected;

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int) override;
	long							OnCommand(unsigned short, unsigned short, HWND) override;
	long							OnSysCommand(unsigned int cmd, LPARAM) override;

private:
	void							RefreshLayout();

	Framework::Win32::CListBox		m_callersList;
};
