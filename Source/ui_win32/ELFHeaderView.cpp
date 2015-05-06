#include <stdio.h>
#include <boost/lexical_cast.hpp>
#include "ELFHeaderView.h"
#include "layout/LayoutStretch.h"
#include "win32/Static.h"
#include "win32/LayoutWindow.h"
#include "lexical_cast_ex.h"
#include "ElfViewRes.h"

CELFHeaderView::CELFHeaderView(HWND hParent, CELF* pELF)
: CDialog(MAKEINTRESOURCE(IDD_ELFVIEW_HEADERVIEW), hParent)
, m_pELF(pELF)
{
	SetClassPtr();
	
	m_pType		= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_TYPE_EDIT));
	m_pMachine	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_MACHINE_EDIT));
	m_pVersion	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_VERSION_EDIT));
	m_pEntry	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_ENTRY_EDIT));
	m_pFlags	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_FLAGS_EDIT));
	m_pSize		= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_SIZE_EDIT));
	m_pPHOffset	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_PHOFFSET_EDIT));
	m_pPHSize	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_PHSIZE_EDIT));
	m_pPHCount	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_PHCOUNT_EDIT));
	m_pSHOffset	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_SHOFFSET_EDIT));
	m_pSHSize	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_SHSIZE_EDIT));
	m_pSHCount	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_SHCOUNT_EDIT));
	m_pSHStrTab = new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_HEADERVIEW_SHSTRTAB_EDIT));

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

	m_pLayout = Framework::CGridLayout::Create(2, 14);

	m_pLayout->SetObject(0,  0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_TYPE_LABEL))));
	m_pLayout->SetObject(0,  1, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_MACHINE_LABEL))));
	m_pLayout->SetObject(0,  2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_VERSION_LABEL))));
	m_pLayout->SetObject(0,  3, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_ENTRY_LABEL))));
	m_pLayout->SetObject(0,  4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_FLAGS_LABEL))));
	m_pLayout->SetObject(0,  5, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_SIZE_LABEL))));
	m_pLayout->SetObject(0,  6, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_PHOFFSET_LABEL))));
	m_pLayout->SetObject(0,  7, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_PHSIZE_LABEL))));
	m_pLayout->SetObject(0,  8, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_PHCOUNT_LABEL))));
	m_pLayout->SetObject(0,  9, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_SHOFFSET_LABEL))));
	m_pLayout->SetObject(0, 10, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_SHSIZE_LABEL))));
	m_pLayout->SetObject(0, 11, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_SHCOUNT_LABEL))));
	m_pLayout->SetObject(0, 12, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_HEADERVIEW_SHSTRTAB_LABEL))));

	m_pLayout->SetObject(1,  0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pType));
	m_pLayout->SetObject(1,  1, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pMachine));
	m_pLayout->SetObject(1,  2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pVersion));
	m_pLayout->SetObject(1,  3, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pEntry));
	m_pLayout->SetObject(1,  4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pFlags));
	m_pLayout->SetObject(1,  5, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pSize));
	m_pLayout->SetObject(1,  6, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pPHOffset));
	m_pLayout->SetObject(1,  7, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pPHSize));
	m_pLayout->SetObject(1,  8, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pPHCount));
	m_pLayout->SetObject(1,  9, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pSHOffset));
	m_pLayout->SetObject(1, 10, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pSHSize));
	m_pLayout->SetObject(1, 11, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pSHCount));
	m_pLayout->SetObject(1, 12, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pSHStrTab));
	m_pLayout->SetObject(1, 13, Framework::CLayoutStretch::Create());

	FillInformation();
	
	RefreshLayout();
}

CELFHeaderView::~CELFHeaderView()
{

}

void CELFHeaderView::FillInformation()
{
	TCHAR sTemp[256];
	const ELFHEADER* pH = &m_pELF->GetHeader();

	switch(pH->nType)
	{
	case CELF::ET_NONE:
		_tcscpy(sTemp, _T("ET_NONE"));
		break;
	case CELF::ET_REL:
		_tcscpy(sTemp, _T("ET_REL"));
		break;
	case CELF::ET_EXEC:
		_tcscpy(sTemp, _T("ET_EXEC"));
		break;
	case CELF::ET_DYN:
		_tcscpy(sTemp, _T("ET_DYN"));
		break;
	case CELF::ET_CORE:
		_tcscpy(sTemp, _T("ET_CORE"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (%i)"), pH->nType);
		break;
	}
	m_pType->SetText(sTemp);

	switch(pH->nCPU)
	{
	case CELF::EM_NONE:
		_tcscpy(sTemp, _T("EM_NONE"));
		break;
	case CELF::EM_M32:
		_tcscpy(sTemp, _T("EM_M32"));
		break;
	case CELF::EM_SPARC:
		_tcscpy(sTemp, _T("EM_SPARC"));
		break;
	case CELF::EM_386:
		_tcscpy(sTemp, _T("EM_386"));
		break;
	case CELF::EM_68K:
		_tcscpy(sTemp, _T("EM_68K"));
		break;
	case CELF::EM_88K:
		_tcscpy(sTemp, _T("EM_88K"));
		break;
	case CELF::EM_860:
		_tcscpy(sTemp, _T("EM_860"));
		break;
	case CELF::EM_MIPS:
		_tcscpy(sTemp, _T("EM_MIPS"));
		break;
	case CELF::EM_ARM:
		_tcscpy(sTemp, _T("EM_ARM"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (%i)"), pH->nCPU);
		break;
	}
	m_pMachine->SetText(sTemp);

	switch(pH->nVersion)
	{
	case CELF::EV_NONE:
		_tcscpy(sTemp, _T("EV_NONE"));
		break;
	case CELF::EV_CURRENT:
		_tcscpy(sTemp, _T("EV_CURRENT"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (%i)"), pH->nVersion);
		break;
	}
	m_pVersion->SetText(sTemp);

	m_pEntry->SetText((_T("0x") + lexical_cast_hex<std::tstring>(pH->nEntryPoint, 8)).c_str());
	m_pPHOffset->SetText((_T("0x") + lexical_cast_hex<std::tstring>(pH->nProgHeaderStart, 8)).c_str());
	m_pPHSize->SetText((_T("0x") + lexical_cast_hex<std::tstring>(pH->nProgHeaderEntrySize, 8)).c_str());
	m_pPHCount->SetText(boost::lexical_cast<std::tstring>(pH->nProgHeaderCount).c_str());
	m_pSHOffset->SetText((_T("0x") + lexical_cast_hex<std::tstring>(pH->nSectHeaderStart, 8)).c_str());
	m_pSHSize->SetText((_T("0x") + lexical_cast_hex<std::tstring>(pH->nSectHeaderEntrySize, 8)).c_str());
	m_pSHCount->SetText(boost::lexical_cast<std::tstring>(pH->nSectHeaderCount).c_str());
	m_pFlags->SetText((_T("0x") + lexical_cast_hex<std::tstring>(pH->nFlags, 8)).c_str());
	m_pSize->SetText((_T("0x") + lexical_cast_hex<std::tstring>(pH->nSize, 8)).c_str());
	m_pSHStrTab->SetText(boost::lexical_cast<std::tstring>(pH->nSectHeaderStringTableIndex).c_str());
}

void CELFHeaderView::RefreshLayout()
{
	{
		RECT rc;
		::GetClientRect(GetParent(), &rc);

		SetPosition(0, 0);
		SetSize(rc.right, rc.bottom);
	}

	{
		RECT rc = GetClientRect();

		SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom + m_pLayout->GetPreferredHeight());

		m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
		m_pLayout->RefreshGeometry();
	}

	Redraw();
}

long CELFHeaderView::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}
