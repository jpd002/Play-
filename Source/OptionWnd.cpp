#include "OptionWnd.h"
#include "PtrMacro.h"
#include "win32/LayoutWindow.h"
#include "win32/MDIChild.h"

#define CLSNAME		_X("OptionWindow")
#define WNDSTYLE	(WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX | WS_CLIPCHILDREN)

using namespace Framework;

template <typename T> 
COptionWnd<T>::COptionWnd(HWND hParent, xchar* sTitle)
{
	RECT rc;

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

	SetRect(&rc, 0, 0, 640, 480);

	Create(NULL, CLSNAME, sTitle, WNDSTYLE, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 1, 1);

	m_pTreeView		= NULL;
	m_pTreeView		= new CTreeView(m_hWnd, &rc, TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_HASLINES);
	m_pContainer	= new CStatic(m_hWnd, &rc);

	m_pLayout = new CHorizontalLayout;
	m_pLayout->InsertObject(new CLayoutWindow(25, 20, 1, 0, m_pTreeView, false));
	m_pLayout->InsertObject(new CLayoutWindow(75, 20, 3, 0, m_pContainer));

	RefreshLayout();
}

template <typename T> 
COptionWnd<T>::~COptionWnd()
{
	DELETEPTR(m_pLayout);
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
	return TRUE;
}

template <typename T>
long COptionWnd<T>::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

template <typename T>
CWindow* COptionWnd<T>::GetContainer()
{
	return m_pContainer;
}

template <typename T>
CTreeView* COptionWnd<T>::GetTreeView()
{
	return m_pTreeView;
}

template <typename T>
void COptionWnd<T>::RefreshLayout()
{
	TVITEM it;
	HTREEITEM hItem;
	RECT rc;
	GetClientRect(&rc);

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
	RECT rc;
	if(hPanel == NULL) return;
	m_pContainer->GetClientRect(&rc);
	SetWindowPos(hPanel, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOZORDER);
	UpdateWindow(hPanel);
}

template <typename T>
void COptionWnd<T>::UpdatePanel(TVITEM* pNew, TVITEM* pOld)
{
	HWND hNew, hOld;
	hNew = (HWND)pNew->lParam;
	hOld = (HWND)pOld->lParam;
	if(hOld != NULL)
	{
		ShowWindow(hOld, SW_HIDE);
		EnableWindow(hOld, FALSE);
	}
	if(hNew != NULL)
	{
		ResizePanel(hNew);
		EnableWindow(hNew, TRUE);
		ShowWindow(hNew, SW_SHOW);
	}
}

template <typename T>
HTREEITEM COptionWnd<T>::InsertOption(HTREEITEM hParent, const xchar* sName, HWND hWnd)
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

template class COptionWnd<Framework::CWindow>;
template class COptionWnd<Framework::CMDIChild>;
