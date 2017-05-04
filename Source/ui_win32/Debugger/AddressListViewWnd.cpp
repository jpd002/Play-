#include "AddressListViewWnd.h"
#include "win32/DpiUtils.h"
#include "../resource.h"
#include "string_format.h"
#include "string_cast.h"

#define WND_STYLE (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CAddressListViewWnd::CAddressListViewWnd(HWND parentWnd)
{
	auto windowRect = Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 700, 400));

	Create(NULL, Framework::Win32::CDefaultWndClass().GetName(), _T(""), WND_STYLE, windowRect, parentWnd, NULL);
	SetClassPtr();

	m_addressListBox = Framework::Win32::CListBox(m_hWnd, Framework::Win32::CRect(0, 0, 10, 10), WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT);

	RefreshLayout();
}

void CAddressListViewWnd::SetTitle(std::tstring title)
{
	SetText(title.c_str());
}

void CAddressListViewWnd::SetAddressList(AddressList addressList)
{
	m_addressListBox.ResetContent();
	for(const auto& address : addressList)
	{
		auto addressString = string_format(_T("0x%08X"), address);
		auto itemIdx = m_addressListBox.AddString(addressString.c_str());
		m_addressListBox.SetItemData(itemIdx, address);
	}
}

long CAddressListViewWnd::OnSize(unsigned int, unsigned int width, unsigned int height)
{
	RefreshLayout();
	return TRUE;
}

long CAddressListViewWnd::OnCommand(unsigned short, unsigned short cmd, HWND senderWnd)
{
	if(CWindow::IsCommandSource(&m_addressListBox, senderWnd))
	{
		switch(cmd)
		{
		case LBN_DBLCLK:
			{
				auto selection = m_addressListBox.GetCurrentSelection();
				if(selection != -1)
				{
					auto selectedAddress = m_addressListBox.GetItemData(selection);
					AddressSelected(selectedAddress);
				}
			}
			break;
		}
	}
	return TRUE;
}

long CAddressListViewWnd::OnSysCommand(unsigned int cmd, LPARAM)
{
	switch(cmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CAddressListViewWnd::RefreshLayout()
{
	auto clientRect = GetClientRect();
	clientRect.Inflate(-5, -5);
	m_addressListBox.SetSizePosition(clientRect);
}
