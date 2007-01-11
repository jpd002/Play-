#include <stdio.h>
#include "ELFSymbolView.h"
#include "PtrMacro.h"

#define CLSNAME _X("CELFSymbolView")

using namespace Framework;

CELFSymbolView::CELFSymbolView(HWND hParent, CELF* pELF)
{
	RECT rc;
	LVCOLUMN col;

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

	SetRect(&rc, 0, 0, 1, 1);

	Create(NULL, CLSNAME, _X(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, &rc, hParent, NULL);
	SetClassPtr();

	m_pELF = pELF;

	m_pListView = new Win32::CListView(m_hWnd, &rc, LVS_REPORT);
	m_pListView->SetExtendedListViewStyle(m_pListView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Name");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Address");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(1, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Size");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(2, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Type");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(3, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Binding");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(4, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _X("Section");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(5, &col);

	RefreshLayout();

	m_pListView->GetClientRect(&rc);

	m_pListView->SetColumnWidth(0, rc.right / 3);
	m_pListView->SetColumnWidth(1, rc.right / 4);
	m_pListView->SetColumnWidth(2, rc.right / 4);
	m_pListView->SetColumnWidth(3, rc.right / 4);
	m_pListView->SetColumnWidth(4, rc.right / 4);
	m_pListView->SetColumnWidth(5, rc.right / 4);

	PopulateList();

	m_pListView->Redraw();
}

CELFSymbolView::~CELFSymbolView()
{
	Destroy();
	DELETEPTR(m_pListView);	
}

long CELFSymbolView::OnSize(unsigned int nType, unsigned int nWidth, unsigned int nHeight)
{
	RefreshLayout();
	return FALSE;
}

void CELFSymbolView::PopulateList()
{
	unsigned int i, nCount;
	LVITEM it;
	ELFSYMBOL* pSym;
	ELFSECTIONHEADER* pSymTab;
	const char* pStrTab;
	xchar sTemp[256];


	pSymTab = m_pELF->FindSection(".symtab");
	if(pSymTab == NULL) return;

	pStrTab = (const char*)m_pELF->GetSectionData(pSymTab->nIndex);
	if(pStrTab == NULL) return;

	pSym = (ELFSYMBOL*)m_pELF->FindSectionData(".symtab");
	nCount = pSymTab->nSize / sizeof(ELFSYMBOL);

	m_pListView->DeleteAllItems();
	
	for(i = 0; i < nCount; i++)
	{
		memset(&it, 0, sizeof(LVITEM));
		m_pListView->InsertItem(&it);
	}

	for(i = 0; i < nCount; i++)
	{
		xconvert(sTemp, (char*)pStrTab + pSym[i].nName, 256);
		m_pListView->SetItemText(i, 0, sTemp);

		xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pSym[i].nValue);
		m_pListView->SetItemText(i, 1, sTemp);

		xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pSym[i].nSize);
		m_pListView->SetItemText(i, 2, sTemp);
	
		switch(pSym[i].nInfo & 0x0F)
		{
		case 0x00:
			xstrcpy(sTemp, _X("STT_NOTYPE"));
			break;
		case 0x01:
			xstrcpy(sTemp, _X("STT_OBJECT"));
			break;
		case 0x02:
			xstrcpy(sTemp, _X("STT_FUNC"));
			break;
		case 0x03:
			xstrcpy(sTemp, _X("STT_SECTION"));
			break;
		case 0x04:
			xstrcpy(sTemp, _X("STT_FILE"));
			break;
		default:
			xsnprintf(sTemp, countof(sTemp), _X("%i"), pSym[i].nInfo & 0x0F);
			break;
		}
		m_pListView->SetItemText(i, 3, sTemp);

		switch((pSym[i].nInfo >> 4) & 0xF)
		{
		case 0x00:
			xstrcpy(sTemp, _X("STB_LOCAL"));
			break;
		case 0x01:
			xstrcpy(sTemp, _X("STB_GLOBAL"));
			break;
		case 0x02:
			xstrcpy(sTemp, _X("STB_WEAK"));
			break;
		default:
			xsnprintf(sTemp, countof(sTemp), _X("%i"), (pSym[i].nInfo >> 4) & 0x0F);
			break;
		}
		m_pListView->SetItemText(i, 4, sTemp);

		switch(pSym[i].nSectionIndex)
		{
		case 0:
			xstrcpy(sTemp, _X("SHN_UNDEF"));
			break;
		case 0xFFF1:
			xstrcpy(sTemp, _X("SHN_ABS"));
			break;
		default:
			xsnprintf(sTemp, countof(sTemp), _X("%i"), pSym[i].nSectionIndex);
			break;
		}
		m_pListView->SetItemText(i, 5, sTemp);
	
	}
}

void CELFSymbolView::RefreshLayout()
{
	RECT rc;
	::GetClientRect(GetParent(), &rc);

	SetPosition(0, 0);
	SetSize(rc.right, rc.bottom);

	GetClientRect(&rc);

	m_pListView->SetPosition(10, 10);
	m_pListView->SetSize(rc.right - 20, rc.bottom - 20);

}
