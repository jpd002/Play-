#include "TabHost.h"
#include <assert.h>

#define WNDSTYLE					(WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CHILD | WS_VISIBLE)

CTabHost::CTabHost(HWND parent, const RECT& rect)
: m_currentSelection(-1)
{
	Create(0, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WNDSTYLE, rect, parent, nullptr);
	SetClassPtr();

	m_tab = std::make_unique<Framework::Win32::CTab>(m_hWnd, rect);
}

CTabHost::~CTabHost()
{

}

Framework::Win32::CWindow* CTabHost::GetTab(unsigned int tabIndex)
{
	assert(tabIndex < m_tabItems.size());
	return m_tabItems[tabIndex];
}

void CTabHost::InsertTab(const TCHAR* title, Framework::Win32::CWindow* tabItem)
{
	m_tab->InsertTab(title);
	m_tabItems.push_back(tabItem);
	if(tabItem)
	{
		tabItem->Show(SW_HIDE);
	}
	if(m_tabItems.size() == 1)
	{
		SetSelection(0);
	}
}

int CTabHost::GetSelection()
{
	return m_currentSelection;
}

void CTabHost::SetSelection(unsigned int tabIndex)
{
	assert(tabIndex < m_tabItems.size());
	m_tab->SetSelection(tabIndex);
	OnTabSelChanged();
}

long CTabHost::OnSize(unsigned int, unsigned int, unsigned int)
{
	if(m_tab)
	{
		m_tab->SetSizePosition(GetClientRect());

		if(m_currentSelection != -1)
		{
			if(auto tabItem = m_tabItems[m_currentSelection])
			{
				tabItem->SetSizePosition(m_tab->GetDisplayAreaRect());
			}
		}
	}
	return TRUE;
}

long CTabHost::OnNotify(WPARAM param, NMHDR* header)
{
	if(CWindow::IsNotifySource(m_tab.get(), header))
	{
		switch(header->code)
		{
		case TCN_SELCHANGE:
			OnTabSelChanged();
			break;
		}
		return FALSE;
	}
	return FALSE;
}

void CTabHost::OnTabSelChanged()
{
	int selectedTab = m_tab->GetSelection();
	if(m_currentSelection != -1)
	{
		auto tabItem(m_tabItems[m_currentSelection]);
		if(tabItem)
		{
			tabItem->Show(SW_HIDE);
		}
	}

	assert(selectedTab < m_tabItems.size());
	m_currentSelection = selectedTab;

	SELCHANGED_INFO selchangedInfo;
	memset(&selchangedInfo, 0, sizeof(SELCHANGED_INFO));
	selchangedInfo.code				= NOTIFICATION_SELCHANGED;
	selchangedInfo.hwndFrom			= m_hWnd;
	selchangedInfo.selectedIndex	= m_currentSelection;
	selchangedInfo.selectedWindow	= m_tabItems[m_currentSelection];
	SendMessage(GetParent(), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&selchangedInfo));

	SetWindowPos(*m_tab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	auto tabItem(m_tabItems[m_currentSelection]);
	if(tabItem)
	{
		tabItem->SetSizePosition(m_tab->GetDisplayAreaRect());
		tabItem->Show(SW_SHOW);
	}
}
