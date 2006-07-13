#ifndef _ABOUTWND_H_
#define _ABOUTWND_H_

#include "win32/Window.h"
#include "win32/Static.h"
#include "win32/Button.h"
#include "VerticalLayout.h"

class CAboutWnd : public Framework::CWindow
{
public:
									CAboutWnd(HWND);
									~CAboutWnd();

protected:
	long							OnSysCommand(unsigned int, LPARAM);

private:
	void							RefreshLayout();

	Framework::CVerticalLayout*		m_pLayout;
	Framework::Win32::CStatic*		m_pImage;
};

#endif
