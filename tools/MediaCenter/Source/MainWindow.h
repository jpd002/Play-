#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "win32/Window.h"
#include "FrameBufferWindow.h"

class CMainWindow : public Framework::Win32::CWindow
{
public:
							CMainWindow();
	virtual					~CMainWindow();

	void					SetImage(unsigned int, unsigned int, uint8*);

private:
	CFrameBufferWindow*		m_frameBufferWindow;
};

#endif
