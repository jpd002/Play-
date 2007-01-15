#include <boost/lexical_cast.hpp>
#include "OsEventViewWnd.h"
#include "win32/Rect.h"
#include "xml/FilteringNodeIterator.h"
#include "xml/Utils.h"
#include "../OsEventManager.h"
#include "../PS2VM.h"
#include "resource.h"

#define CLSNAME		_T("OsEventViewWnd")

using namespace Framework;
using namespace boost;
using namespace std;

#define ID_REFRESH	(0xDEAF + 0)

COsEventViewWnd::COsEventViewWnd(HWND hParent)
{
	m_pEvents = NULL;

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
		Win32::CRect(0, 0, 320, 240), hParent, NULL);
	SetClassPtr();

	m_pList = new Win32::CListView(m_hWnd, Win32::CRect(0, 0, 1, 1), LVS_REPORT | LVS_OWNERDATA);
	m_pList->SetExtendedListViewStyle(m_pList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_pToolBar = new Win32::CToolBar(m_hWnd);
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
	col.pszText = _T("Event Type");
	col.mask	= LVCF_TEXT;
	m_pList->InsertColumn(1, &col);
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
	return FALSE;
}

void COsEventViewWnd::Update()
{
	Xml::CNode* pRootNode;

	pRootNode = COsEventManager::GetInstance().GetEvents();
	if(pRootNode != NULL)
	{
		Xml::CNode* pEventsNode;

		pEventsNode = pRootNode->Search("Events");
		if(pEventsNode != NULL)
		{
			unsigned int nCount = 0;
			for(Xml::CFilteringNodeIterator itEvent(pEventsNode, "Event");
				!itEvent.IsEnd(); itEvent++)
			{
				nCount++;
			}

			m_pList->SetItemCount(nCount);
		}

		return;
	}

	m_pList->SetItemCount(0);

		/*
	Xml::CNode* pRootNode;
	Xml::CNode* pEventsNode;

	m_pList->DeleteAllItems();

	pRootNode = COsEventManager::GetInstance().GetEvents();
	if(pRootNode == NULL) return;

	pEventsNode = pRootNode->Search("Events");
	if(pEventsNode != NULL)
	{
		m_pList->SetRedraw(false);

		for(Xml::CFilteringNodeIterator itEvent(pEventsNode, "Event");
			!itEvent.IsEnd(); itEvent++)
		{
			LVITEM Item;
			int nTime, nItemIndex, nThreadId;

			if(!Xml::GetAttributeIntValue(*itEvent, "Time", &nTime)) continue;
			if(!Xml::GetAttributeIntValue(*itEvent, "ThreadId", &nThreadId)) continue;

			//Add the current function
			memset(&Item, 0, sizeof(LVITEM));
			Item.pszText	= _T("");
			Item.iItem		= m_pList->GetItemCount();
			Item.mask		= LVIF_TEXT | LVIF_PARAM;
			Item.lParam		= nTime;
			nItemIndex = m_pList->InsertItem(&Item);

			m_pList->SetItemText(nItemIndex, 0, lexical_cast<tstring>(nThreadId).c_str());
		}

		m_pList->SetRedraw(true);
	}

	delete pRootNode;
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

		m_pList->SetColumnWidth(0, rc.right / 2);
		m_pList->SetColumnWidth(1, rc.right / 2);
	}
}
