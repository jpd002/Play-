#include "PixelBufferViewOverlay.h"
#include "win32/DpiUtils.h"

#define WNDSTYLE	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)

CPixelBufferViewOverlay::CPixelBufferViewOverlay(HWND parentWnd)
{
	Create(0, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WNDSTYLE, 
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 64, 32)), parentWnd, nullptr);
	SetClassPtr();

	m_saveButton = std::make_unique<Framework::Win32::CButton>(_T("Save"), m_hWnd, 
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 32, 32)));
	m_fitButton = std::make_unique<Framework::Win32::CButton>(_T("Fit"), m_hWnd, 
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(32, 0, 64, 32)));
}

CPixelBufferViewOverlay::~CPixelBufferViewOverlay()
{

}

long CPixelBufferViewOverlay::OnCommand(unsigned short, unsigned short, HWND wndFrom)
{
	if(CWindow::IsCommandSource(m_saveButton.get(), wndFrom))
	{
		SendMessage(GetParent(), WM_COMMAND, COMMAND_SAVE << 16, reinterpret_cast<LPARAM>(m_hWnd));
	}
	else if(CWindow::IsCommandSource(m_fitButton.get(), wndFrom))
	{
		SendMessage(GetParent(), WM_COMMAND, COMMAND_FIT << 16, reinterpret_cast<LPARAM>(m_hWnd));
	}
	return TRUE;
}
