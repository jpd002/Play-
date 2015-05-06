#include <stdio.h>
#include "ELFSectionView.h"
#include "layout/GridLayout.h"
#include "win32/LayoutWindow.h"
#include "string_cast.h"
#include "ElfViewRes.h"

CELFSectionView::CELFSectionView(HWND hParent, CELF* pELF)
: CDialog(MAKEINTRESOURCE(IDD_ELFVIEW_SECTIONVIEW), hParent)
, m_nSection(-1)
, m_pELF(pELF)
, m_dynamicSectionListView(NULL)
, m_memoryView(NULL)
{
	SetClassPtr();

	m_pType					= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_TYPE_EDIT));
	m_pFlags				= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_FLAGS_EDIT));
	m_pAddress				= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_ADDRESS_EDIT));
	m_pOffset				= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_OFFSET_EDIT));
	m_pSize					= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_SIZE_EDIT));
	m_pLink					= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_LINK_EDIT));
	m_pInfo					= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_INFO_EDIT));
	m_pAlignment			= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_ALIGN_EDIT));
	m_pEntrySize			= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_SECTIONVIEW_ENTRYSIZE_EDIT));
	m_contentsPlaceHolder	= new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_CONTENTS_PLACEHOLDER));

	//Create content views
	{
		m_memoryView = new CMemoryViewPtr(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));

		m_dynamicSectionListView = new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT);
		m_dynamicSectionListView->SetExtendedListViewStyle(m_dynamicSectionListView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);
	}

	RECT columnEditBoxSize;
	SetRect(&columnEditBoxSize, 0, 0, 70, 12);
	MapDialogRect(m_hWnd, &columnEditBoxSize);
	unsigned int columnEditBoxWidth = columnEditBoxSize.right - columnEditBoxSize.left;
	unsigned int columnEditBoxHeight = columnEditBoxSize.bottom - columnEditBoxSize.top;

	RECT columnLabelSize;
	SetRect(&columnLabelSize, 0, 0, 70, 8);
	MapDialogRect(m_hWnd, &columnLabelSize);
	unsigned int columnLabelWidth = columnLabelSize.right - columnLabelSize.left;
	unsigned int columnLabelHeight = columnLabelSize.bottom - columnLabelSize.top;

	Framework::GridLayoutPtr pSubLayout0 = Framework::CGridLayout::Create(2, 9);

	pSubLayout0->SetObject(0, 0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_TYPE_LABEL))));
	pSubLayout0->SetObject(0, 1, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_FLAGS_LABEL))));
	pSubLayout0->SetObject(0, 2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_ADDRESS_LABEL))));
	pSubLayout0->SetObject(0, 3, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_OFFSET_LABEL))));
	pSubLayout0->SetObject(0, 4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_SIZE_LABEL))));
	pSubLayout0->SetObject(0, 5, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_LINK_LABEL))));
	pSubLayout0->SetObject(0, 6, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_INFO_LABEL))));
	pSubLayout0->SetObject(0, 7, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_ALIGN_LABEL))));
	pSubLayout0->SetObject(0, 8, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_SECTIONVIEW_ENTRYSIZE_LABEL))));

	pSubLayout0->SetObject(1, 0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pType));
	pSubLayout0->SetObject(1, 1, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pFlags));
	pSubLayout0->SetObject(1, 2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pAddress));
	pSubLayout0->SetObject(1, 3, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pOffset));
	pSubLayout0->SetObject(1, 4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pSize));
	pSubLayout0->SetObject(1, 5, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pLink));
	pSubLayout0->SetObject(1, 6, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pInfo));
	pSubLayout0->SetObject(1, 7, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pAlignment));
	pSubLayout0->SetObject(1, 8, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pEntrySize));

	pSubLayout0->SetVerticalStretch(0);

	m_pLayout = Framework::CVerticalLayout::Create();
	m_pLayout->InsertObject(pSubLayout0);
	m_pLayout->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(200, 200, 1, 1, m_contentsPlaceHolder));

	RefreshLayout();

	CreateDynamicSectionListViewColumns();
}

CELFSectionView::~CELFSectionView()
{
	delete m_memoryView;
	delete m_dynamicSectionListView;
}

long CELFSectionView::OnSize(unsigned int nType, unsigned int nWidth, unsigned int nHeight)
{
	RefreshLayout();
	return TRUE;
}

long CELFSectionView::OnSetFocus()
{
	m_memoryView->SetFocus();
	return FALSE;
}

void CELFSectionView::SetSectionIndex(uint16 sectionIndex)
{
	m_nSection = sectionIndex;
	FillInformation();
}

void CELFSectionView::RefreshLayout()
{
	{
		RECT rc;
		::GetClientRect(GetParent(), &rc);

		SetPosition(0, 0);
		SetSize(rc.right, rc.bottom);
	}

	{
		RECT rc = GetClientRect();

		SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

		m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
		m_pLayout->RefreshGeometry();
	}

	{
		RECT rc = m_contentsPlaceHolder->GetWindowRect();
		ScreenToClient(m_hWnd, reinterpret_cast<POINT*>(&rc) + 0);
		ScreenToClient(m_hWnd, reinterpret_cast<POINT*>(&rc) + 1);

		m_memoryView->SetSizePosition(rc);
		m_dynamicSectionListView->SetSizePosition(rc);
	}

	Redraw();
}

void CELFSectionView::FillInformation()
{
	TCHAR sTemp[256];
	ELFSECTIONHEADER* pH = m_pELF->GetSection(m_nSection);

	switch(pH->nType)
	{
	case CELF::SHT_NULL:
		_tcscpy(sTemp, _T("SHT_NULL"));
		break;
	case CELF::SHT_PROGBITS:
		_tcscpy(sTemp, _T("SHT_PROGBITS"));
		break;
	case CELF::SHT_SYMTAB:
		_tcscpy(sTemp, _T("SHT_SYMTAB"));
		break;
	case CELF::SHT_STRTAB:
		_tcscpy(sTemp, _T("SHT_STRTAB"));
		break;
	case CELF::SHT_HASH:
		_tcscpy(sTemp, _T("SHT_HASH"));
		break;
	case CELF::SHT_DYNAMIC:
		_tcscpy(sTemp, _T("SHT_DYNAMIC"));
		break;
	case CELF::SHT_NOTE:
		_tcscpy(sTemp, _T("SHT_NOTE"));
		break;
	case CELF::SHT_NOBITS:
		_tcscpy(sTemp, _T("SHT_NOBITS"));
		break;
	case CELF::SHT_REL:
		_tcscpy(sTemp, _T("SHT_REL"));
		break;
	case CELF::SHT_DYNSYM:
		_tcscpy(sTemp, _T("SHT_DYNSYM"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (0x%0.8X)"), pH->nType);
		break;
	}
	m_pType->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nFlags);
	if(pH->nFlags & 0x7)
	{
		_tcscat(sTemp, _T(" ("));
		if(pH->nFlags & 0x01)
		{
			_tcscat(sTemp, _T("SHF_WRITE"));
			if(pH->nFlags & 0x6)
			{
				_tcscat(sTemp, _T(" | "));
			}
		}
		if(pH->nFlags & 0x2)
		{
			_tcscat(sTemp, _T("SHF_ALLOC"));
			if(pH->nFlags & 0x04)
			{
				_tcscat(sTemp, _T(" | "));
			}
		}
		if(pH->nFlags & 0x04)
		{
			_tcscat(sTemp, _T("SHF_EXECINSTR"));
		}
		_tcscat(sTemp, _T(")"));
	}
	m_pFlags->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nStart);
	m_pAddress->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nOffset);
	m_pOffset->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nSize);
	m_pSize->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("%i"), pH->nIndex);
	m_pLink->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("%i"), pH->nInfo);
	m_pInfo->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nAlignment);
	m_pAlignment->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nOther);
	m_pEntrySize->SetText(sTemp);

	if(pH->nType == CELF::SHT_DYNAMIC)
	{
		FillDynamicSectionListView();
		m_dynamicSectionListView->Show(SW_SHOW);
		m_memoryView->Show(SW_HIDE);
	}
	else if(pH->nType != CELF::SHT_NOBITS)
	{
		m_memoryView->SetData((void*)m_pELF->GetSectionData(m_nSection), pH->nSize);
		m_memoryView->Show(SW_SHOW);
		m_dynamicSectionListView->Show(SW_HIDE);
	}
	else
	{
		m_memoryView->Show(SW_HIDE);
		m_dynamicSectionListView->Show(SW_HIDE);
	}
}

void CELFSectionView::CreateDynamicSectionListViewColumns()
{
	LVCOLUMN col;

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Type");
	col.mask		= LVCF_TEXT;
	m_dynamicSectionListView->InsertColumn(0, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Value");
	col.mask		= LVCF_TEXT;
	m_dynamicSectionListView->InsertColumn(1, col);

	RECT rc = m_dynamicSectionListView->GetClientRect();

	m_dynamicSectionListView->SetColumnWidth(0, 1 * rc.right / 3);
	m_dynamicSectionListView->SetColumnWidth(1, 2 * rc.right / 3);
}

void CELFSectionView::FillDynamicSectionListView()
{
	m_dynamicSectionListView->DeleteAllItems();

	const ELFSECTIONHEADER* pH = m_pELF->GetSection(m_nSection);
	const uint32* dynamicData = reinterpret_cast<const uint32*>(m_pELF->GetSectionData(m_nSection));
	const char* stringTable = (pH->nOther != -1) ? reinterpret_cast<const char*>(m_pELF->GetSectionData(pH->nIndex)) : NULL;

	for(unsigned int i = 0; i < pH->nSize; i += 8, dynamicData += 2)
	{
		uint32 tag		= *(dynamicData + 0);
		uint32 value	= *(dynamicData + 1);

		if(tag == 0) break;

		TCHAR tempTag[256];
		TCHAR tempVal[256];

		switch(tag)
		{
		case CELF::DT_NEEDED:
			_tcscpy(tempTag, _T("DT_NEEDED"));
			_tcscpy(tempVal, string_cast<std::tstring>(stringTable + value).c_str());
			break;
		case CELF::DT_PLTRELSZ:
			_tcscpy(tempTag, _T("DT_PLTRELSZ"));
			_sntprintf(tempVal, countof(tempVal), _T("0x%0.8X"), value);
			break;
		case CELF::DT_PLTGOT:
			_tcscpy(tempTag, _T("DT_PLTGOT"));
			_sntprintf(tempVal, countof(tempVal), _T("0x%0.8X"), value);
			break;
		case CELF::DT_HASH:
			_tcscpy(tempTag, _T("DT_HASH"));
			_sntprintf(tempVal, countof(tempVal), _T("0x%0.8X"), value);
			break;
		case CELF::DT_STRTAB:
			_tcscpy(tempTag, _T("DT_STRTAB"));
			_sntprintf(tempVal, countof(tempVal), _T("0x%0.8X"), value);
			break;
		case CELF::DT_SYMTAB:
			_tcscpy(tempTag, _T("DT_SYMTAB"));
			_sntprintf(tempVal, countof(tempVal), _T("0x%0.8X"), value);
			break;
		case CELF::DT_SONAME:
			_tcscpy(tempTag, _T("DT_SONAME"));
			_tcscpy(tempVal, string_cast<std::tstring>(stringTable + value).c_str());
			break;
		case CELF::DT_SYMBOLIC:
			_tcscpy(tempTag, _T("DT_SYMBOLIC"));
			_tcscpy(tempVal, _T(""));
			break;
		default:
			_sntprintf(tempTag, countof(tempTag), _T("Unknown (0x%0.8X)"), tag);
			_sntprintf(tempVal, countof(tempVal), _T("0x%0.8X"), value);
			break;
		}

		LVITEM item;
		memset(&item, 0, sizeof(LVITEM));
		item.iItem = m_dynamicSectionListView->GetItemCount();
		unsigned int itemIndex = m_dynamicSectionListView->InsertItem(item);

		m_dynamicSectionListView->SetItemText(itemIndex, 0, tempTag);
		m_dynamicSectionListView->SetItemText(itemIndex, 1, tempVal);
	}
}
