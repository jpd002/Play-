#include "FindCallersViewWnd.h"
#include "win32/DpiUtils.h"
#include "../resource.h"
#include "string_format.h"
#include "string_cast.h"

#define WND_STYLE (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

#define MAX_ADDRESS 0x02000000

CFindCallersViewWnd::CFindCallersViewWnd(HWND parentWnd)
{
	auto windowRect = Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 700, 400));

	Create(NULL, Framework::Win32::CDefaultWndClass().GetName(), _T(""), WND_STYLE, windowRect, parentWnd, NULL);
	SetClassPtr();

	m_callersList = Framework::Win32::CListBox(m_hWnd, Framework::Win32::CRect(0, 0, 10, 10), WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT);

	RefreshLayout();
}

CFindCallersViewWnd::~CFindCallersViewWnd()
{

}

long CFindCallersViewWnd::OnSize(unsigned int, unsigned int width, unsigned int height)
{
	RefreshLayout();
	return TRUE;
}

long CFindCallersViewWnd::OnCommand(unsigned short, unsigned short cmd, HWND senderWnd)
{
	if(CWindow::IsCommandSource(&m_callersList, senderWnd))
	{
		switch(cmd)
		{
		case LBN_DBLCLK:
			{
				auto selection = m_callersList.GetCurrentSelection();
				if(selection != -1)
				{
					auto selectedAddress = m_callersList.GetItemData(selection);
					AddressSelected(selectedAddress);
				}
			}
			break;
		}
	}
	return TRUE;
}

long CFindCallersViewWnd::OnSysCommand(unsigned int cmd, LPARAM)
{
	switch(cmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CFindCallersViewWnd::RefreshLayout()
{
	auto clientRect = GetClientRect();
	clientRect.Inflate(-5, -5);
	m_callersList.SetSizePosition(clientRect);
}

void CFindCallersViewWnd::FindCallers(CMIPS* context, uint32 address)
{
	m_callersList.ResetContent();

	auto functionName = context->m_Functions.Find(address);

	auto title = std::tstring();
	if(functionName)
	{
		title = string_format(_T("Find Callers For '%s' (0x%0.8X)"), 
			string_cast<std::tstring>(functionName).c_str(), address);
	}
	else
	{
		title = string_format(_T("Find Callers For 0x%0.8X"), address);
	}

	SetText(title.c_str());

	for(int i = 0; i < MAX_ADDRESS; i += 4)
	{
		uint32 opcode = context->m_pMemoryMap->GetInstruction(i);
		uint32 ea = context->m_pArch->GetInstructionEffectiveAddress(context, i, opcode);
		if(ea == address)
		{
			auto callerName = string_format(_T("0x%0.8X"), i);
			auto itemIdx = m_callersList.AddString(callerName.c_str());
			m_callersList.SetItemData(itemIdx, i);
		}
	}
}
