#ifndef _CALLSTACKWND_H_
#define _CALLSTACKWND_H_

#include "win32/MDIChild.h"
#include "win32/ListView.h"
#include "Event.h"

class CMIPS;

class CCallStackWnd : public Framework::CMDIChild
{
public:
									CCallStackWnd(HWND, CMIPS*);
									~CCallStackWnd();
	Framework::CEvent<uint32>		m_OnFunctionDblClick;

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnSysCommand(unsigned int, LPARAM);
	long							OnNotify(WPARAM, NMHDR*);

private:
	void							RefreshLayout();
	void							CreateColumns();
	void							Update();
	void							OnListDblClick();

	void							OnMachineStateChange(int);
	void							OnRunningStateChange(int);

	Framework::CEventHandler<int>*	m_pOnMachineStateChangeHandler;
	Framework::CEventHandler<int>*	m_pOnRunningStateChangeHandler;

	CMIPS*							m_pCtx;
	Framework::CListView*			m_pList;
};

#endif
