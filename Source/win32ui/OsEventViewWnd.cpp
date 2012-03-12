#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "OsEventViewWnd.h"
#include "win32/Rect.h"
#include "xml/FilteringNodeIterator.h"
#include "xml/Utils.h"
#include "../PS2VM.h"
#include "resource.h"

#define CLSNAME		_T("OsEventViewWnd")

#define ID_REFRESH	(0xDEAF + 0)

COsEventViewWnd::COsEventViewWnd(HWND hParent)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		RegisterClassEx(&wc);
	}

	Create(NULL, CLSNAME, _T("OS Events"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, 
		Framework::Win32::CRect(0, 0, 320, 240), hParent, NULL);
	SetClassPtr();

	m_pList = new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT | LVS_OWNERDATA);
	m_pList->SetExtendedListViewStyle(m_pList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_pToolBar = new Framework::Win32::CToolBar(m_hWnd);
	m_pToolBar->LoadStandardImageList(IDB_STD_SMALL_COLOR);
	m_pToolBar->InsertImageButton(STD_UNDO, ID_REFRESH);
	m_pToolBar->SetButtonToolTipText(ID_REFRESH, _T("Refresh"));

	CreateColumns();

	RefreshLayout();
}

COsEventViewWnd::~COsEventViewWnd()
{

}

void COsEventViewWnd::CreateColumns()
{
	LVCOLUMN col;
	RECT rc;

	m_pList->GetClientRect(&rc);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Thread Id");
	col.mask	= LVCF_TEXT;
	m_pList->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Description");
	col.mask	= LVCF_TEXT;
	m_pList->InsertColumn(1, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Address");
	col.mask	= LVCF_TEXT;
	m_pList->InsertColumn(2, &col);
}

long COsEventViewWnd::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

long COsEventViewWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

long COsEventViewWnd::OnCommand(unsigned short nId, unsigned short nCmd, HWND hWndFrom)
{
	switch(nId)
	{
	case ID_REFRESH:
		Update();
		break;
	}

	return TRUE;
}

long COsEventViewWnd::OnNotify(WPARAM wParam, NMHDR* pHdr)
{
	if(m_pToolBar != NULL)
	{
		m_pToolBar->ProcessNotify(wParam, pHdr);
	}
	if(m_pList != NULL)
	{
		m_pList->ProcessGetDisplayInfo(pHdr, boost::bind(&COsEventViewWnd::GetDisplayInfoCallback, this, _1));
		if(pHdr->hwndFrom == m_pList->m_hWnd)
		{
			switch(pHdr->code)
			{
			case NM_DBLCLK:
				OnListDblClick();
				break;
			}
		}
	}
	return FALSE;
}

void COsEventViewWnd::Update()
{
	m_ListItems.clear();
/*
	Xml::CNode* pRootNode;

	pRootNode = COsEventManager::GetInstance().GetEvents();
	if(pRootNode != NULL)
	{
		Xml::CNode* pEventsNode;

		pEventsNode = pRootNode->Search("Events");
		if(pEventsNode != NULL)
		{
			for(Xml::CFilteringNodeIterator itEvent(pEventsNode, "Event");
				!itEvent.IsEnd(); itEvent++)
			{
				int nId, nThreadId, nEventType, nAddress;
				const char* sDescription;

				if(!Xml::GetAttributeIntValue(*itEvent, "Time",			&nId))				continue;
				if(!Xml::GetAttributeIntValue(*itEvent, "EventType",	&nEventType))		continue;
				if(!Xml::GetAttributeIntValue(*itEvent, "ThreadId",		&nThreadId))		continue;
				if(!Xml::GetAttributeIntValue(*itEvent, "Address",		&nAddress))			continue;
				if(!Xml::GetAttributeStringValue(*itEvent, "Description", &sDescription))	continue;

				LISTITEM Item;
				Item.nThreadId		= nThreadId;
//				Item.sDescription	= lexical_cast<tstring>(nEventType);
				Item.sDescription	= string_cast<tstring>(sDescription);
				Item.nAddress		= nAddress;

				m_ListItems[nId] = Item;
			}

			m_pList->SetItemCount(static_cast<int>(m_ListItems.size()));
		}
	}
	else
	{
		m_pList->SetItemCount(0);
	}
*/
}

void COsEventViewWnd::RefreshLayout()
{
	if(m_pToolBar != NULL && m_pList != NULL)
	{
		RECT rc;
		RECT ToolBarRect;

		GetClientRect(&rc);

		m_pToolBar->Resize();

		m_pToolBar->GetWindowRect(&ToolBarRect);
		ScreenToClient(m_hWnd, reinterpret_cast<POINT*>(&ToolBarRect) + 0);
		ScreenToClient(m_hWnd, reinterpret_cast<POINT*>(&ToolBarRect) + 1);

		m_pList->SetPosition(0, ToolBarRect.bottom);
		m_pList->SetSize(rc.right, rc.bottom - ToolBarRect.bottom);

		m_pList->GetClientRect(&rc);

		m_pList->SetColumnWidth(0, rc.right * 1 / 5);
		m_pList->SetColumnWidth(1, rc.right * 2 / 5);
		m_pList->SetColumnWidth(2, rc.right * 2 / 5);
	}
}

void COsEventViewWnd::GetDisplayInfoCallback(LVITEM* pItem)
{
	if((pItem->mask & LVIF_TEXT) == 0) return;
	if(static_cast<unsigned int>(pItem->iItem) >= m_ListItems.size()) return;

	LISTITEM& Item(m_ListItems[pItem->iItem]);

	switch(pItem->iSubItem)
	{
	case 0:
		_tcsncpy(pItem->pszText, boost::lexical_cast<std::tstring>(Item.nThreadId).c_str(), pItem->cchTextMax);
		break;
	case 1:
		_tcsncpy(pItem->pszText, Item.sDescription.c_str(), pItem->cchTextMax);
		break;
	case 2:
		_tcsncpy(pItem->pszText, (_T("0x") + lexical_cast_hex<std::tstring>(Item.nAddress, 8)).c_str(), pItem->cchTextMax);
		break;
	}
}

void COsEventViewWnd::OnListDblClick()
{
	int nSelection = m_pList->GetSelection();
	if(nSelection == -1) return;
	if(static_cast<unsigned int>(nSelection) >= m_ListItems.size()) return;

	LISTITEM& Item(m_ListItems[nSelection]);
	
	OnEventDblClick(Item.nAddress);
}
