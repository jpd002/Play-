#include "win32/Rect.h"
#include "MainWindow.h"

#define APP_NAME			_T("MPlayer")
#define CLSNAME				_T("MainWindow")
#define WNDSTYLE		    (WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX)
#define WNDSTYLEEX		    (0)

using namespace Framework;

CMainWindow::CMainWindow() :
m_frameBufferWindow(NULL)
{
    if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&Win32::CWindow::MakeWndClass(CLSNAME));
	}

    Win32::CRect clientRect(0, 0, 640, 480);
    AdjustWindowRectEx(clientRect, WNDSTYLE, FALSE, WNDSTYLEEX);
    Create(WNDSTYLEEX, CLSNAME, APP_NAME, WNDSTYLE, clientRect, NULL, NULL);
    SetClassPtr();

	m_frameBufferWindow = new CFrameBufferWindow(m_hWnd);
	m_frameBufferWindow->SetViewportSize(640, 480);
}

CMainWindow::~CMainWindow()
{
	delete m_frameBufferWindow;
}

void CMainWindow::SetImage(unsigned int width, unsigned int height, uint8* image)
{
	m_frameBufferWindow->SetImage(width, height, image);
}
