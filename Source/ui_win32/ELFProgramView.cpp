#include <stdio.h>
#include "ELFProgramView.h"
#include "layout/LayoutStretch.h"
#include "win32/Static.h"
#include "win32/LayoutWindow.h"
#include "ElfViewRes.h"

CELFProgramView::CELFProgramView(HWND hParent, CELF* pELF)
: CDialog(MAKEINTRESOURCE(IDD_ELFVIEW_PROGRAMVIEW), hParent)
, m_nProgram(-1)
, m_pELF(pELF)
{
	SetClassPtr();

	m_pType		= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_TYPE_EDIT));
	m_pOffset	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_OFFSET_EDIT));
	m_pVAddr	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_VADDR_EDIT));
	m_pPAddr	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_PADDR_EDIT));
	m_pFileSize	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_FILESIZE_EDIT));
	m_pMemSize	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_MEMSIZE_EDIT));
	m_pFlags	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_FLAGS_EDIT));
	m_pAlign	= new Framework::Win32::CEdit(GetItem(IDC_ELFVIEW_PROGRAMVIEW_ALIGN_EDIT));

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

	m_pLayout = Framework::CGridLayout::Create(2, 9);

	m_pLayout->SetObject(0, 0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_TYPE_LABEL))));
	m_pLayout->SetObject(0, 1, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_OFFSET_LABEL))));
	m_pLayout->SetObject(0, 2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_VADDR_LABEL))));
	m_pLayout->SetObject(0, 3, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_PADDR_LABEL))));
	m_pLayout->SetObject(0, 4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_FILESIZE_LABEL))));
	m_pLayout->SetObject(0, 5, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_MEMSIZE_LABEL))));
	m_pLayout->SetObject(0, 6, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_FLAGS_LABEL))));
	m_pLayout->SetObject(0, 7, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ELFVIEW_PROGRAMVIEW_ALIGN_LABEL))));

	m_pLayout->SetObject(1, 0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pType));
	m_pLayout->SetObject(1, 1, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pOffset));
	m_pLayout->SetObject(1, 2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pVAddr));
	m_pLayout->SetObject(1, 3, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pPAddr));
	m_pLayout->SetObject(1, 4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pFileSize));
	m_pLayout->SetObject(1, 5, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pMemSize));
	m_pLayout->SetObject(1, 6, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pFlags));
	m_pLayout->SetObject(1, 7, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_pAlign));
	m_pLayout->SetObject(1, 8, Framework::CLayoutStretch::Create());

	RefreshLayout();
}

CELFProgramView::~CELFProgramView()
{

}

long CELFProgramView::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

void CELFProgramView::SetProgramIndex(uint16 programIndex)
{
	m_nProgram = programIndex;
	FillInformation();
}

void CELFProgramView::FillInformation()
{
	TCHAR sTemp[256];
	ELFPROGRAMHEADER* pH = m_pELF->GetProgram(m_nProgram);

	switch(pH->nType)
	{
	case CELF::PT_NULL:
		_tcscpy(sTemp, _T("PT_NULL"));
		break;
	case CELF::PT_LOAD:
		_tcscpy(sTemp, _T("PT_LOAD"));
		break;
	case CELF::PT_DYNAMIC:
		_tcscpy(sTemp, _T("PT_DYNAMIC"));
		break;
	case CELF::PT_INTERP:
		_tcscpy(sTemp, _T("PT_INTERP"));
		break;
	case CELF::PT_NOTE:
		_tcscpy(sTemp, _T("PT_NOTE"));
		break;
	case CELF::PT_SHLIB:
		_tcscpy(sTemp, _T("PT_SHLIB"));
		break;
	case CELF::PT_PHDR:
		_tcscpy(sTemp, _T("PT_PHDR"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (0x%0.8X)"), pH->nType);
		break;
	}
	m_pType->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nOffset);
	m_pOffset->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nVAddress);
	m_pVAddr->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nPAddress);
	m_pPAddr->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nFileSize);
	m_pFileSize->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nMemorySize);
	m_pMemSize->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nFlags);
	m_pFlags->SetText(sTemp);

	_sntprintf(sTemp, countof(sTemp), _T("0x%0.8X"), pH->nAlignment);
	m_pAlign->SetText(sTemp);
}

void CELFProgramView::RefreshLayout()
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
