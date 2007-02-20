#include <stdio.h>
#include "ELFSymbolView.h"
#include "PtrMacro.h"
#include <boost/lexical_cast.hpp>
#include "string_cast.h"
#include "lexical_cast_ex.h"

#define CLSNAME _T("CELFSymbolView")

using namespace Framework;
using namespace std;
using namespace boost;

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

	Create(NULL, CLSNAME, _T(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, &rc, hParent, NULL);
	SetClassPtr();

	m_pELF = pELF;

	m_pListView = new Win32::CListView(m_hWnd, &rc, LVS_REPORT);
	m_pListView->SetExtendedListViewStyle(m_pListView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Name");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Address");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(1, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Size");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(2, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Type");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(3, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Binding");
	col.mask		= LVCF_TEXT;
	m_pListView->InsertColumn(4, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Section");
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
	unsigned int nCount;
	LVITEM it;
	ELFSYMBOL* pSym;
	ELFSECTIONHEADER* pSymTab;
	const char* pStrTab;

	pSymTab = m_pELF->FindSection(".symtab");
	if(pSymTab == NULL) return;

	pStrTab = (const char*)m_pELF->GetSectionData(pSymTab->nIndex);
	if(pStrTab == NULL) return;

	pSym = (ELFSYMBOL*)m_pELF->FindSectionData(".symtab");
	nCount = pSymTab->nSize / sizeof(ELFSYMBOL);

	m_pListView->DeleteAllItems();
	
	for(unsigned int i = 0; i < nCount; i++)
	{
		memset(&it, 0, sizeof(LVITEM));
		m_pListView->InsertItem(&it);
	}

	for(unsigned int i = 0; i < nCount; i++)
	{
    	TCHAR sTemp[256];

        m_pListView->SetItemText(i, 0, string_cast<tstring>(pStrTab + pSym[i].nName).c_str());
		m_pListView->SetItemText(i, 1, (_T("0x") + lexical_cast_hex<tstring>(pSym[i].nValue, 8)).c_str());
		m_pListView->SetItemText(i, 2, (_T("0x") + lexical_cast_hex<tstring>(pSym[i].nSize, 8)).c_str());
	
		switch(pSym[i].nInfo & 0x0F)
		{
		case 0x00:
			_tcscpy(sTemp, _T("STT_NOTYPE"));
			break;
		case 0x01:
			_tcscpy(sTemp, _T("STT_OBJECT"));
			break;
		case 0x02:
			_tcscpy(sTemp, _T("STT_FUNC"));
			break;
		case 0x03:
			_tcscpy(sTemp, _T("STT_SECTION"));
			break;
		case 0x04:
			_tcscpy(sTemp, _T("STT_FILE"));
			break;
		default:
			_sntprintf(sTemp, countof(sTemp), _T("%i"), pSym[i].nInfo & 0x0F);
			break;
		}
		m_pListView->SetItemText(i, 3, sTemp);

		switch((pSym[i].nInfo >> 4) & 0xF)
		{
		case 0x00:
			_tcscpy(sTemp, _T("STB_LOCAL"));
			break;
		case 0x01:
			_tcscpy(sTemp, _T("STB_GLOBAL"));
			break;
		case 0x02:
			_tcscpy(sTemp, _T("STB_WEAK"));
			break;
		default:
			_sntprintf(sTemp, countof(sTemp), _T("%i"), (pSym[i].nInfo >> 4) & 0x0F);
			break;
		}
		m_pListView->SetItemText(i, 4, sTemp);

		switch(pSym[i].nSectionIndex)
		{
		case 0:
			_tcscpy(sTemp, _T("SHN_UNDEF"));
			break;
		case 0xFFF1:
			_tcscpy(sTemp, _T("SHN_ABS"));
			break;
		default:
			_sntprintf(sTemp, countof(sTemp), _T("%i"), pSym[i].nSectionIndex);
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
