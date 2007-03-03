#ifndef _REGVIEWWND_H_
#define _REGVIEWWND_H_

#include "win32/MDIChild.h"
#include "NiceTabs.h"

class CMIPS;

class CRegViewWnd : public Framework::Win32::CMDIChild, public boost::signals::trackable
{
public:
						CRegViewWnd(HWND, CMIPS*);
	virtual				~CRegViewWnd();

protected:
	long				OnSize(unsigned int, unsigned int, unsigned int);
	long				OnSysCommand(unsigned int, LPARAM);

private:
	enum
	{
		MAXTABS			= 4,
	};

	void				SetCurrentView(unsigned int);
	void				OnTabChange(unsigned int);
	void				RefreshLayout();

	CWindow*			m_pRegView[MAXTABS];
	CWindow*			m_pCurrent;
	CNiceTabs*			m_pTabs;
};

#endif