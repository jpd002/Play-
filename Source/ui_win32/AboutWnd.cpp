#include <zlib.h>
#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include "AboutWnd.h"
#include "layout/LayoutStretch.h"
#include "layout/HorizontalLayout.h"
#include "win32/LayoutWindow.h"
#include "win32/DefaultWndClass.h"
#include "string_cast.h"
#include "string_format.h"
#include "resource.h"
#include "../PS2VM.h"

#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)
#define SCALE(x)	MulDiv(x, ydpi, 96)

CAboutWnd::CAboutWnd(HWND parentWnd) 
: Framework::Win32::CModalWindow(parentWnd)
{
	if(parentWnd != NULL)
	{
		EnableWindow(parentWnd, FALSE);
	}

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);

	Create(WNDSTYLEEX, Framework::Win32::CDefaultWndClass().GetName(), _T("About Play!"), WNDSTYLE,
		Framework::Win32::CRect(0, 0, SCALE(360), SCALE(125)), parentWnd, NULL);
	SetClassPtr();

	m_pImage = new Framework::Win32::CStatic(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), SS_ICON);
	m_pImage->SetIcon(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY), IMAGE_ICON, 48, 48, 0));

	auto date(string_cast<std::tstring>(__DATE__));
	auto zlibVersion(string_cast<std::tstring>(ZLIB_VERSION));

	auto version = string_format(_T("Version %s (%s) - zlib v%s - boost v%s"),
		APP_VERSIONSTR, date.c_str(), zlibVersion.c_str(), GetBoostVersion().c_str());

	auto pSubLayout0 = Framework::CVerticalLayout::Create(3);
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, SCALE(30), new Framework::Win32::CStatic(m_hWnd, version.c_str(), SS_EDITCONTROL)));
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, SCALE(15), new Framework::Win32::CStatic(m_hWnd, _T("Written by Jean-Philip Desjardins"))));
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, SCALE(15), new Framework::Win32::CStatic(m_hWnd, _T("jean-philip.desjardins@polymtl.ca"))));
	pSubLayout0->InsertObject(Framework::CLayoutStretch::Create());
	pSubLayout0->SetHorizontalStretch(1);

	auto pSubLayout1 = Framework::CVerticalLayout::Create();
	pSubLayout1->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(48, 48, 0, 0, m_pImage));
	pSubLayout1->InsertObject(Framework::CLayoutStretch::Create());

	auto pSubLayout2 = Framework::CHorizontalLayout::Create(10);
	pSubLayout2->InsertObject(pSubLayout1);
	pSubLayout2->InsertObject(pSubLayout0);
	pSubLayout2->SetVerticalStretch(0);

	m_pLayout = Framework::CVerticalLayout::Create();
	m_pLayout->InsertObject(pSubLayout2);
	m_pLayout->InsertObject(Framework::CLayoutStretch::Create());

	RefreshLayout();
}

CAboutWnd::~CAboutWnd()
{

}

void CAboutWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

std::tstring CAboutWnd::GetBoostVersion()
{
	return 
		boost::lexical_cast<std::tstring>(BOOST_VERSION / 100000) + _T(".") +
		boost::lexical_cast<std::tstring>(BOOST_VERSION / 100 % 1000) + _T(".") +
		boost::lexical_cast<std::tstring>(BOOST_VERSION % 100);
}
