#include <stdio.h>
#include "FunctionsView.h"
#include "HorizontalLayout.h"
#include "win32/LayoutWindow.h"
#include "win32/InputBox.h"
#include "LayoutStretch.h"
#include "PtrMacro.h"

#define CLSNAME _X("FunctionsView")

using namespace Framework;

CFunctionsView::CFunctionsView(HWND hParent, CMIPS* pCtx)
{
	RECT rc;
	CHorizontalLayout* pSubLayout0;

	m_pCtx = pCtx;
	m_pELF = NULL;

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

	SetRect(&rc, 0, 0, 320, 240);

	Create(NULL, CLSNAME, _X("Functions"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 0, 0);

	m_pList = new CListView(m_hWnd, &rc, LVS_REPORT | LVS_SINGLESEL | LVS_SORTASCENDING | LVS_SHOWSELALWAYS);
	m_pList->SetExtendedListViewStyle(m_pList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	CreateListColumns();

	m_pNew		= new CButton(_X("New..."), m_hWnd, &rc);
	m_pDelete	= new CButton(_X("Delete"), m_hWnd, &rc);
	m_pImport	= new CButton(_X("Load ELF symbols"), m_hWnd, &rc);

	pSubLayout0 = new CHorizontalLayout;
	pSubLayout0->InsertObject(new CLayoutStretch);
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pNew));
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pDelete));
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pImport));

	m_pLayout = new CVerticalLayout;
	m_pLayout->InsertObject(new CLayoutWindow(1, 1, 1, 1, m_pList));
	m_pLayout->InsertObject(pSubLayout0);

	SetSize(469, 612);

	SetELF(NULL);

	RefreshLayout();

}

CFunctionsView::~CFunctionsView()
{
	DELETEPTR(m_pLayout);
}

void CFunctionsView::Refresh()
{
	RefreshList();
}

long CFunctionsView::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
	RefreshLayout();
	return TRUE;
}

long CFunctionsView::OnNotify(WPARAM wParam, NMHDR* pH)
{
	if(pH->hwndFrom == m_pList->m_hWnd)
	{
		NMLISTVIEW* pN;
		pN = (NMLISTVIEW*)pH;
		switch(pN->hdr.code)
		{
		case NM_DBLCLK:
			OnListDblClick();
			break;
		}
	}

	return FALSE;
}

long CFunctionsView::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	if(hSender == m_pNew->m_hWnd)
	{
		OnNewClick();
	}
	if(hSender == m_pDelete->m_hWnd)
	{
		OnDeleteClick();
	}
	if(hSender == m_pImport->m_hWnd)
	{
		OnImportClick();
	}
	return TRUE;
}

long CFunctionsView::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CFunctionsView::CreateListColumns()
{
	LVCOLUMN col;

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Name");
	col.mask		= LVCF_TEXT;
	m_pList->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Address");
	col.mask		= LVCF_TEXT;
	m_pList->InsertColumn(1, &col);
}

void CFunctionsView::ResizeListColumns()
{
	RECT rc;

	m_pList->GetClientRect(&rc);

	m_pList->SetColumnWidth(0, rc.right * 2 / 3);
	m_pList->SetColumnWidth(1, rc.right * 1 / 3);
}

void CFunctionsView::RefreshLayout()
{
	RECT rc;

	if(m_pLayout == NULL) return;

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();

	ResizeListColumns();
}

void CFunctionsView::RefreshList()
{
	LVITEM it;
	unsigned int nCount, i;
	uint32 nAddress;
	char* sTag;
	CList<char>::ITERATOR itTag;
	xchar sTemp[256];

	m_pList->DeleteAllItems();

	for(itTag = m_pCtx->m_Functions.m_Tag.Begin(); itTag.HasNext(); itTag++)
	{
		sTag = (*itTag);

		xconvert(sTemp, sTag, 256);

		memset(&it, 0, sizeof(LVITEM));
		it.pszText	= sTemp;
		it.lParam	= itTag.GetKey();
		it.mask		= LVIF_PARAM | LVIF_TEXT;
		m_pList->InsertItem(&it);
	}

	nCount = m_pList->GetItemCount();

	for(i = 0; i < nCount; i++)
	{
		nAddress = m_pList->GetItemData(i);
		xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), nAddress);
		m_pList->SetItemText(i, 1, sTemp);
	}
}

void CFunctionsView::SetELF(CELF* pELF)
{
	m_pELF = pELF;
	if(pELF == NULL)
	{
		m_pImport->Enable(false);
	}
	else
	{
		m_pImport->Enable(true);
	}
}

void CFunctionsView::OnListDblClick()
{
	int nItem;
	uint32 nAddress;
	nItem = m_pList->GetSelection();
	if(nItem == -1) return;
	nAddress = (uint32)m_pList->GetItemData(nItem);
	m_OnFunctionDblClick.Notify(nAddress);
}

void CFunctionsView::OnNewClick()
{
	CInputBox* pInput;
	bool bQuit;
	xchar sNameX[256];
	const xchar* sValue;
	char sName[256];
	uint32 nAddress;

	pInput = new CInputBox(_X("New Function"), _X("New Function Name:"), _X(""));
	sValue = pInput->GetValue(m_hWnd);

	bQuit = (sValue == NULL);
	if(sValue != NULL)
	{
		xstrncpy(sNameX, sValue, 255);	
	}

	delete pInput;

	if(bQuit) return;

	pInput = new CInputBox(_X("New Function"), _X("New Function Address:"), _X("00000000"));
	sValue = pInput->GetValue(m_hWnd);

	bQuit = (sValue == NULL);
	if(sValue != NULL)
	{
		xsscanf(sValue, _X("%x"), &nAddress);
		if((nAddress & 0x3) != 0x0)
		{
			MessageBox(m_hWnd, _X("Invalid address."), NULL, 16);
			bQuit = true;
		}
	}

	delete pInput;

	if(bQuit) return;

	xconvert(sName, sNameX, 256);

	m_pCtx->m_Functions.InsertTag(nAddress, sName);

	RefreshList();
	m_OnFunctionsStateChange.Notify(NULL);
}

void CFunctionsView::OnImportClick()
{
	unsigned int i, nCount;
	ELFSYMBOL* pSym;
	ELFSECTIONHEADER* pSymTab;
	const char* pStrTab;

	if(m_pELF == NULL) return;

	pSymTab = m_pELF->FindSection(".symtab");
	if(pSymTab == NULL) return;

	pStrTab = (const char*)m_pELF->GetSectionData(pSymTab->nIndex);
	if(pStrTab == NULL) return;

	pSym = (ELFSYMBOL*)m_pELF->FindSectionData(".symtab");
	nCount = pSymTab->nSize / sizeof(ELFSYMBOL);

	for(i = 0; i < nCount; i++)
	{
		if((pSym[i].nInfo & 0x0F) != 0x02) continue;
		m_pCtx->m_Functions.InsertTag(pSym[i].nValue, (char*)pStrTab + pSym[i].nName);
	}

	RefreshList();

	m_OnFunctionsStateChange.Notify(NULL);

	printf("FunctionsView: Symbolic information found and loaded.\r\n");
}

void CFunctionsView::OnDeleteClick()
{
	int nItem;
	uint32 nAddress;

	nItem = m_pList->GetSelection();
	if(nItem == -1) return;
	if(MessageBox(m_hWnd, _X("Delete this function?"), NULL, MB_ICONQUESTION | MB_YESNO) != IDYES)
	{
		return;
	}

	nAddress = m_pList->GetItemData(nItem);
	m_pCtx->m_Functions.InsertTag(nAddress, NULL);
	RefreshList();

	m_OnFunctionsStateChange.Notify(NULL);
}
