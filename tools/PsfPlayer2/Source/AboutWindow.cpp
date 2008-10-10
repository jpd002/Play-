#include "AboutWindow.h"
#include "AppDef.h"
#include "layout/LayoutEngine.h"
#include "win32/Rect.h"
#include "win32/Static.h"

#define CLSNAME		_T("AboutWindow")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

using namespace std;
using namespace Framework;

CAboutWindow::CAboutWindow(HWND parent) : 
CModalWindow(parent)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&MakeWndClass(CLSNAME));
	}

	Create(WNDSTYLEEX, CLSNAME, _T("About"), WNDSTYLE, Win32::CRect(0, 0, 400, 140), parent, NULL);
	SetClassPtr();

	tstring aboutText;
	aboutText  = tstring(APP_NAME) + _T(" v") + tstring(APP_VERSIONSTR) + _T("\r\n");
	aboutText += _T("\r\n");
	aboutText += tstring(_T("By Jean-Philip Desjardins")) + _T("\r\n");
	aboutText += _T("\r\n");
	aboutText += tstring(_T("Thanks to Neill Corlett for creating the PSF format and also for his ADSR and reverb analysis.")) + _T("\r\n");

	m_layout = Win32::CLayoutWindow::CreateTextBoxBehavior(100, 60, new Win32::CStatic(m_hWnd, aboutText.c_str()));

	RefreshLayout();
}

CAboutWindow::~CAboutWindow()
{

}

void CAboutWindow::RefreshLayout()
{
	RECT rc = GetClientRect();
	m_layout->SetRect(rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);
	m_layout->RefreshGeometry();
}
