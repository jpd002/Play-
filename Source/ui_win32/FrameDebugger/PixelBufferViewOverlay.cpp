#include "PixelBufferViewOverlay.h"
#include "win32/DpiUtils.h"
#include "string_cast.h"

#define WNDSTYLE	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)

CPixelBufferViewOverlay::CPixelBufferViewOverlay(HWND parentWnd)
{
	Create(0, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WNDSTYLE, 
		Framework::Win32::CRect(0, 0, 1, 1), parentWnd, nullptr);
	SetClassPtr();

	static const unsigned int buttonSize = 32;
	m_saveButton = Framework::Win32::CButton(_T("Save"), m_hWnd,
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, buttonSize, buttonSize)));
	m_fitButton = Framework::Win32::CButton(_T("Fit"), m_hWnd, 
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(buttonSize, 0, buttonSize * 2, buttonSize)));
	m_pixelBufferComboBox = Framework::Win32::CComboBox(m_hWnd,
		Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, buttonSize, buttonSize * 2, buttonSize + 1)), CBS_DROPDOWNLIST);

	auto buttonsRect = Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, buttonSize * 2, buttonSize));
	auto comboBoxRect = m_pixelBufferComboBox.GetClientRect();
	SetWindowPos(m_hWnd, NULL, 0, 0, buttonsRect.Width(), buttonsRect.Height() + comboBoxRect.Height(), SWP_NOMOVE | SWP_NOZORDER);
}

void CPixelBufferViewOverlay::SetPixelBufferTitles(StringList titles)
{
	m_pixelBufferComboBox.ResetContent();
	for(const auto& title : titles)
	{
		m_pixelBufferComboBox.AddString(string_cast<std::tstring>(title).c_str());
	}
	m_pixelBufferComboBox.SetSelection(0);
}

int CPixelBufferViewOverlay::GetSelectedPixelBufferIndex()
{
	return m_pixelBufferComboBox.GetSelection();
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
	else if(CWindow::IsCommandSource(&m_pixelBufferComboBox, wndFrom))
	{
		switch(cmd)
		{
		case CBN_SELCHANGE:
			SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(0, COMMAND_PIXELBUFFER_CHANGED), reinterpret_cast<LPARAM>(m_hWnd));
			break;
		}
	}
	return TRUE;
}
