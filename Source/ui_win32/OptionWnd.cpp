#include "OptionWnd.h"
#include "PtrMacro.h"
#include "win32/LayoutWindow.h"
#include "win32/MDIChild.h"

#define CLSNAME		_T("OptionWindow")
#define WNDSTYLE	(WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX | WS_CLIPCHILDREN)

using namespace Framework;

#define SCALE(x) MulDiv(x, ydpi, 96)

template <typename T> 
COptionWnd<T>::COptionWnd(HWND hParent, const TCHAR* sTitle)
: m_pTreeView(nullptr)
, m_pContainer(nullptr)
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

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);

	Create(NULL, CLSNAME, sTitle, WNDSTYLE, Framework::Win32::CRect(0, 0, SCALE(640), SCALE(480)), hParent, NULL);
	SetClassPtr();

	m_pTreeView		= new Win32::CTreeView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_HASLINES);
	m_pContainer	= new Win32::CStatic(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));

	m_pLayout = CHorizontalLayout::Create();
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateCustomBehavior(25, SCALE(20), 1, 0, m_pTreeView, false));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateCustomBehavior(75, SCALE(20), 3, 0, m_pContainer));

	RefreshLayout();
}

template <typename T> 
COptionWnd<T>::~COptionWnd()
{

}

template <typename T>
void COptionWnd<T>::OnItemAppearing(HTREEITEM item)
{

}

template <typename T> 
long COptionWnd<T>::OnNotify(WPARAM wParam, NMHDR* pH)
{
	if(m_pTreeView != NULL)
	{
		if(pH->hwndFrom == m_pTreeView->m_hWnd)
		{
			NMTREEVIEW* pN;
			pN = (NMTREEVIEW*)pH;
			switch(pN->hdr.code)
			{
			case TVN_SELCHANGED:
				UpdatePanel(&pN->itemNew, &pN->itemOld);
				return FALSE;
				break;
			}
		}
	}

	return FALSE;
}

template <typename T>
long COptionWnd<T>::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

template <typename T>
Win32::CWindow* COptionWnd<T>::GetContainer()
{
	return m_pContainer;
}

template <typename T>
Win32::CTreeView* COptionWnd<T>::GetTreeView()
{
	return m_pTreeView;
}

template <typename T>
void COptionWnd<T>::RefreshLayout()
{
	TVITEM it;
	HTREEITEM hItem;
	RECT rc = GetClientRect();

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	hItem = m_pTreeView->GetSelection();
	if(hItem != NULL)
	{
		it.mask = TVIF_PARAM;
		m_pTreeView->GetItem(hItem, &it);
		ResizePanel((HWND)it.lParam);
	}

	Redraw();
}

template <typename T>
void COptionWnd<T>::ResizePanel(HWND hPanel)
{
	if(hPanel == NULL) return;
	RECT rc = m_pContainer->GetClientRect();
	SetWindowPos(hPanel, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOZORDER);
	UpdateWindow(hPanel);
}

template <typename T>
void COptionWnd<T>::UpdatePanel(TVITEM* pNew, TVITEM* pOld)
{
	HWND hNew = reinterpret_cast<HWND>(pNew->lParam);
	HWND hOld = reinterpret_cast<HWND>(pOld->lParam);
	if(hOld != NULL)
	{
		ShowWindow(hOld, SW_HIDE);
		EnableWindow(hOld, FALSE);
	}
	OnItemAppearing(pNew->hItem);
	if(hNew != NULL)
	{
		ResizePanel(hNew);
		EnableWindow(hNew, TRUE);
		ShowWindow(hNew, SW_SHOW);
	}
}

template <typename T>
HTREEITEM COptionWnd<T>::InsertOption(HTREEITEM hParent, const TCHAR* sName, HWND hWnd)
{
	TVINSERTSTRUCT s;
	s.hParent		= hParent;
	s.hInsertAfter	= TVI_LAST;
	s.item.pszText	= (LPTSTR)sName;
	s.item.lParam	= (LPARAM)hWnd;
	s.item.mask		= TVIF_TEXT | TVIF_PARAM;
	return m_pTreeView->InsertItem(&s);
}

template <typename T>
void COptionWnd<T>::DeleteAllOptions()
{
	m_pTreeView->DeleteAllItems();
}

template class COptionWnd<Framework::Win32::CWindow>;
template class COptionWnd<Framework::Win32::CMDIChild>;
