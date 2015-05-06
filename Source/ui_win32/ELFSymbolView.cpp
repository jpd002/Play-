#include <stdio.h>
#include "ELFSymbolView.h"
#include "PtrMacro.h"
#include <boost/lexical_cast.hpp>
#include "win32/Header.h"
#include "win32/Rect.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"

#define CLSNAME _T("CELFSymbolView")

CELFSymbolView::CELFSymbolView(HWND hParent, CELF* pELF)
: m_pELF(pELF)
, m_listView(NULL)
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

	Create(NULL, CLSNAME, _T(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, Framework::Win32::CRect(0, 0, 1, 1), hParent, NULL);
	SetClassPtr();

	m_listView = new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT | LVS_OWNERDATA);
	m_listView->SetExtendedListViewStyle(m_listView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	LVCOLUMN col;
	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Name");
	col.mask		= LVCF_TEXT;
	m_listView->InsertColumn(0, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Address");
	col.mask		= LVCF_TEXT;
	m_listView->InsertColumn(1, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Size");
	col.mask		= LVCF_TEXT;
	m_listView->InsertColumn(2, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Type");
	col.mask		= LVCF_TEXT;
	m_listView->InsertColumn(3, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Binding");
	col.mask		= LVCF_TEXT;
	m_listView->InsertColumn(4, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Section");
	col.mask		= LVCF_TEXT;
	m_listView->InsertColumn(5, col);

	RefreshLayout();

	RECT rc = m_listView->GetClientRect();

	m_listView->SetColumnWidth(0, rc.right / 3);
	m_listView->SetColumnWidth(1, rc.right / 4);
	m_listView->SetColumnWidth(2, rc.right / 4);
	m_listView->SetColumnWidth(3, rc.right / 4);
	m_listView->SetColumnWidth(4, rc.right / 4);
	m_listView->SetColumnWidth(5, rc.right / 4);

	PopulateList();

	m_listView->Redraw();
}

CELFSymbolView::~CELFSymbolView()
{
	DELETEPTR(m_listView);
}

long CELFSymbolView::OnSize(unsigned int nType, unsigned int nWidth, unsigned int nHeight)
{
	RefreshLayout();
	return FALSE;
}

long CELFSymbolView::OnNotify(WPARAM wParam, NMHDR* hdr)
{
	if(IsNotifySource(m_listView, hdr))
	{
		if(hdr->code == LVN_GETDISPINFO)
		{
			LV_DISPINFO* dispInfo = reinterpret_cast<LV_DISPINFO*>(hdr);
			GetItemInfo(&dispInfo->item);
		}

		if(hdr->code == LVN_COLUMNCLICK)
		{
			NMLISTVIEW* notice(reinterpret_cast<NMLISTVIEW*>(hdr));

			if(notice->iSubItem == 0)
			{
				if(m_sortState == SORT_STATE_NAME_ASC || m_sortState == SORT_STATE_NAME_DESC)
				{
					std::reverse(m_items.begin(), m_items.end());
					m_sortState = (m_sortState == SORT_STATE_NAME_ASC) ? SORT_STATE_NAME_DESC : SORT_STATE_NAME_ASC;
				}
				else
				{
					std::sort(m_items.begin(), m_items.end(), ItemNameComparer);
					m_sortState = SORT_STATE_NAME_ASC;
				}

				m_listView->Redraw();
			}

			if(notice->iSubItem == 1)
			{
				if(m_sortState == SORT_STATE_ADDRESS_ASC || m_sortState == SORT_STATE_ADDRESS_DESC)
				{
					std::reverse(m_items.begin(), m_items.end());
					m_sortState = (m_sortState == SORT_STATE_ADDRESS_ASC) ? SORT_STATE_ADDRESS_DESC : SORT_STATE_ADDRESS_ASC;
				}
				else
				{
					std::sort(m_items.begin(), m_items.end(), ItemAddressComparer);
					m_sortState = SORT_STATE_ADDRESS_ASC;
				}

				m_listView->Redraw();
			}

			//Update columns
			{
				Framework::Win32::CHeader header(m_listView->GetHeader());

				//Column 0
				{
					HDITEM item;
					memset(&item, 0, sizeof(HDITEM));
					item.mask = HDI_FORMAT;
					header.GetItem(0, &item);
					item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
					if(m_sortState == SORT_STATE_NAME_ASC) item.fmt |= HDF_SORTUP;
					if(m_sortState == SORT_STATE_NAME_DESC) item.fmt |= HDF_SORTDOWN;
					header.SetItem(0, &item);
				}

				//Column 1
				{
					HDITEM item;
					memset(&item, 0, sizeof(HDITEM));
					item.mask = HDI_FORMAT;
					header.GetItem(1, &item);
					item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
					if(m_sortState == SORT_STATE_ADDRESS_ASC) item.fmt |= HDF_SORTUP;
					if(m_sortState == SORT_STATE_ADDRESS_DESC) item.fmt |= HDF_SORTDOWN;
					header.SetItem(1, &item);
				}
			}
		}
	}
	return FALSE;
}

int CELFSymbolView::ItemNameComparer(const ITEM& item1, const ITEM& item2)
{
	return item1.name < item2.name;
}

int CELFSymbolView::ItemAddressComparer(const ITEM& item1, const ITEM& item2)
{
	return item1.address < item2.address;
}

void CELFSymbolView::RefreshLayout()
{
	{
		RECT rc;
		::GetClientRect(GetParent(), &rc);

		SetPosition(0, 0);
		SetSize(rc.right, rc.bottom);
	}

	{
		RECT rc = GetClientRect();

		m_listView->SetPosition(10, 10);
		m_listView->SetSize(rc.right - 20, rc.bottom - 20);
	}
}

void CELFSymbolView::PopulateList()
{
	m_sortState = SORT_STATE_NONE;
	const char* sectionName = ".symtab";

	ELFSECTIONHEADER* pSymTab = m_pELF->FindSection(sectionName);
	if(pSymTab == NULL) return;

	const char* pStrTab = (const char*)m_pELF->GetSectionData(pSymTab->nIndex);
	if(pStrTab == NULL) return;

	ELFSYMBOL* pSym = (ELFSYMBOL*)m_pELF->FindSectionData(sectionName);
	unsigned int nCount = pSymTab->nSize / sizeof(ELFSYMBOL);

	m_items.resize(nCount);
	for(unsigned int i = 0; i < nCount; i++)
	{
		ITEM& item(m_items[i]);
		TCHAR sTemp[256];

		item.name		= string_cast<std::tstring>(pStrTab + pSym[i].nName);
		item.address	= _T("0x") + lexical_cast_hex<std::tstring>(pSym[i].nValue, 8);
		item.size		= _T("0x") + lexical_cast_hex<std::tstring>(pSym[i].nSize, 8);

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
		item.type = sTemp;

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
		item.binding = sTemp;

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
		item.section = sTemp;
	}

	m_listView->SetItemCount(nCount);
}

void CELFSymbolView::GetItemInfo(LVITEM* outItem) const
{
	if(outItem->iItem >= m_items.size()) return;
	const ITEM& item(m_items[outItem->iItem]);
	const PooledString* itemText(NULL);
	switch(outItem->iSubItem)
	{
	case 0:
		itemText = &item.name;
		break;
	case 1:
		itemText = &item.address;
		break;
	case 2:
		itemText = &item.size;
		break;
	case 3:
		itemText = &item.type;
		break;
	case 4:
		itemText = &item.binding;
		break;
	case 5:
		itemText = &item.section;
		break;
	}
	outItem->pszText = const_cast<TCHAR*>(static_cast<const std::tstring&>(*itemText).c_str());
}
