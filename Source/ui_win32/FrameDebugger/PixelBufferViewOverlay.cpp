#include "PixelBufferViewOverlay.h"
#include "win32/DpiUtils.h"

#define WNDSTYLE	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)

CPixelBufferViewOverlay::CPixelBufferViewOverlay(HWND parentWnd)
{
	Create(0, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WNDSTYLE, 
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 64, 32)), parentWnd, nullptr);
	SetClassPtr();

	m_saveButton = Framework::Win32::CButton(_T("Save"), m_hWnd,
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 32, 32)));
	m_fitButton = Framework::Win32::CButton(_T("Fit"), m_hWnd, 
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(32, 0, 64, 32)));
}

long CPixelBufferViewOverlay::OnCommand(unsigned short, unsigned short cmd, HWND wndFrom)
{
	if(CWindow::IsCommandSource(&m_saveButton, wndFrom))
	{
		SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(0, COMMAND_SAVE), reinterpret_cast<LPARAM>(m_hWnd));
	}
	else if(CWindow::IsCommandSource(&m_fitButton, wndFrom))
	{
		SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(0, COMMAND_FIT), reinterpret_cast<LPARAM>(m_hWnd));
	}
	return TRUE;
}
