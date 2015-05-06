#ifndef _THREADSVIEWWND_H_
#define _THREADSVIEWWND_H_

#include "win32/ListView.h"
#include "win32/ToolBar.h"
#include "../VirtualMachine.h"
#include "../MIPS.h"
#include "../BiosDebugInfoProvider.h"
#include "win32/MDIChild.h"
#include "Types.h"

class CThreadsViewWnd : public Framework::Win32::CMDIChild, public boost::signals2::trackable
{
public:
											CThreadsViewWnd(HWND, CVirtualMachine&);
	virtual									~CThreadsViewWnd();

	void									SetContext(CMIPS*, CBiosDebugInfoProvider*);

	boost::signals2::signal<void (uint32)>	OnGotoAddress;

protected:
	long									OnSize(unsigned int, unsigned int, unsigned int);
	long									OnSysCommand(unsigned int, LPARAM);
	long									OnNotify(WPARAM, NMHDR*);

private:
	void									CreateColumns();
	void									Update();
	void									RefreshLayout();
	void									OnListDblClick();

	Framework::Win32::CListView				m_listView;

	CMIPS*									m_context;
	CBiosDebugInfoProvider*					m_biosDebugInfoProvider;
};

#endif
