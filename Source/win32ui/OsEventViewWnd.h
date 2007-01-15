#ifndef _OSEVENTVIEWWND_H_
#define _OSEVENTVIEWWND_H_

#include "win32/ListView.h"
#include "win32/ToolBar.h"
#include "win32/MDIChild.h"
#include "xml/Node.h"

class COsEventViewWnd : public Framework::CMDIChild
{
public:
									COsEventViewWnd(HWND);
	virtual							~COsEventViewWnd();

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnSysCommand(unsigned int, LPARAM);
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnNotify(WPARAM, NMHDR*);

private:
	void							CreateColumns();
	void							Update();
	void							RefreshLayout();

	Framework::Win32::CListView*	m_pList;
	Framework::Win32::CToolBar*		m_pToolBar;
	Framework::Xml::CNode*			m_pEvents;
};

#endif
