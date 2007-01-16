#ifndef _OSEVENTVIEWWND_H_
#define _OSEVENTVIEWWND_H_

#include <boost/signal.hpp>
#include "win32/ListView.h"
#include "win32/ToolBar.h"
#include "win32/MDIChild.h"
#include "xml/Node.h"
#include "Types.h"

class COsEventViewWnd : public Framework::CMDIChild
{
public:
									COsEventViewWnd(HWND);
	virtual							~COsEventViewWnd();
	boost::signal<void (uint32)>	m_OnEventDblClick;

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnSysCommand(unsigned int, LPARAM);
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnNotify(WPARAM, NMHDR*);

private:
	struct LISTITEM
	{
		unsigned int	nThreadId;
		std::tstring	sDescription;
		uint32			nAddress;
	};

	typedef std::map<unsigned int, LISTITEM> ListItemMap;

	void							CreateColumns();
	void							Update();
	void							RefreshLayout();
	void							GetDisplayInfoCallback(LVITEM*);
	void							OnListDblClick();

	Framework::Win32::CListView*	m_pList;
	Framework::Win32::CToolBar*		m_pToolBar;
	ListItemMap						m_ListItems;
};

#endif
