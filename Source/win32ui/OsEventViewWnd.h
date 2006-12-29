#ifndef _OSEVENTVIEWWND_H_
#define _OSEVENTVIEWWND_H_

#include <boost/signal.hpp>
#include "win32/ListView.h"
#include "win32/MDIChild.h"
#include "xml/Node.h"

class COsEventViewWnd : public Framework::CMDIChild, public boost::signals::trackable
{
public:
							COsEventViewWnd(HWND);
	virtual					~COsEventViewWnd();

protected:
	long					OnSize(unsigned int, unsigned int, unsigned int);
	long					OnSysCommand(unsigned int, LPARAM);

private:
	void					CreateColumns();
	void					Update();
	void					RefreshLayout();

	Framework::CListView*	m_pList;
	Xml::CNode*				m_pEvents;
};

#endif
