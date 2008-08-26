#ifndef _ABOUTWINDOW_H_
#define _ABOUTWINDOW_H_

#include "win32/ModalWindow.h"
#include "win32/Layouts.h"

class CAboutWindow : public Framework::Win32::CModalWindow
{
public:
								CAboutWindow(HWND);
	virtual						~CAboutWindow();

private:
	void						RefreshLayout();

	Framework::LayoutObjectPtr	m_layout;
};

#endif
