#ifndef _ABOUTWND_H_
#define _ABOUTWND_H_

#include "win32/Window.h"
#include "win32/Static.h"
#include "win32/Button.h"
#include "layout/VerticalLayout.h"

class CAboutWnd : public Framework::Win32::CWindow
{
public:
									CAboutWnd(HWND);
									~CAboutWnd();

protected:
	long							OnSysCommand(unsigned int, LPARAM);

private:
	void							RefreshLayout();
    std::tstring                    GetBoostVersion();

    Framework::FlatLayoutPtr        m_pLayout;
	Framework::Win32::CStatic*      m_pImage;
};

#endif
