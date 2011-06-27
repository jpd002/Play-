#include <stdio.h>
#include "ELFSectionView.h"
#include "PtrMacro.h"
#include "layout/GridLayout.h"
#include "win32/Static.h"
#include "win32/LayoutWindow.h"

#define CLSNAME		_T("CELFSectionView")

using namespace Framework;

CELFSectionView::CELFSectionView(HWND hParent, CELF* pELF)
: m_nSection(-1)
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

	m_pELF = pELF;

	SetRect(&rc, 0, 0, 1, 1);

	Create(NULL, CLSNAME, _T(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, &rc, hParent, NULL);
	SetClassPtr();

	m_pType			= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pFlags		= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pAddress		= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pOffset		= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pSize			= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pLink			= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pInfo			= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pAlignment	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pEntrySize	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);

	m_pData = new CMemoryViewPtr(m_hWnd, &rc);

    GridLayoutPtr pSubLayout0 = CGridLayout::Create(2, 9);

	pSubLayout0->SetObject(0, 0, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Type:"))));
	pSubLayout0->SetObject(0, 1, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Flags:"))));
	pSubLayout0->SetObject(0, 2, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Address:"))));
	pSubLayout0->SetObject(0, 3, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("File Offset:"))));
	pSubLayout0->SetObject(0, 4, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Size:"))));
	pSubLayout0->SetObject(0, 5, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Section Link:"))));
	pSubLayout0->SetObject(0, 6, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Info:"))));
	pSubLayout0->SetObject(0, 7, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Alignment:"))));
	pSubLayout0->SetObject(0, 8, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Entry Size:"))));

	pSubLayout0->SetObject(1, 0, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pType));
	pSubLayout0->SetObject(1, 1, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pFlags));
	pSubLayout0->SetObject(1, 2, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pAddress));
	pSubLayout0->SetObject(1, 3, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pOffset));
	pSubLayout0->SetObject(1, 4, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSize));
	pSubLayout0->SetObject(1, 5, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pLink));
	pSubLayout0->SetObject(1, 6, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pInfo));
	pSubLayout0->SetObject(1, 7, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pAlignment));
	pSubLayout0->SetObject(1, 8, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pEntrySize));

	pSubLayout0->SetVerticalStretch(0);

    m_pLayout = CVerticalLayout::Create();
	m_pLayout->InsertObject(pSubLayout0);
    m_pLayout->InsertObject(Win32::CLayoutWindow::CreateCustomBehavior(200, 200, 1, 1, m_pData));

	RefreshLayout();
}

CELFSectionView::~CELFSectionView()
{
    Destroy();
}

long CELFSectionView::OnSize(unsigned int nType, unsigned int nWidth, unsigned int nHeight)
{
	RefreshLayout();
	return TRUE;
}

long CELFSectionView::OnSetFocus()
{
	m_pData->SetFocus();
	return FALSE;
}

void CELFSectionView::SetSectionIndex(uint16 sectionIndex)
{
	m_nSection = sectionIndex;
	FillInformation();
}

void CELFSectionView::FillInformation()
{
	TCHAR sTemp[256];
	ELFSECTIONHEADER* pH = m_pELF->GetSection(m_nSection);

	switch(pH->nType)
	{
	case 0x00:
		_tcscpy(sTemp, _T("SHT_NULL"));
		break;
	case 0x01:
		_tcscpy(sTemp, _T("SHT_PROGBITS"));
		break;
	case 0x02:
		_tcscpy(sTemp, _T("SHT_SYMTAB"));
		break;
	case 0x03:
		_tcscpy(sTemp, _T("SHT_STRTAB"));
		break;
	case 0x08:
		_tcscpy(sTemp, _T("SHT_NOBITS"));
		break;
    case 0x09:
        _tcscpy(sTemp, _T("SHT_REL"));
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

	if(pH->nType != 0x08)
	{
		m_pData->SetData((void*)m_pELF->GetSectionData(m_nSection), pH->nSize);
	}
}

void CELFSectionView::RefreshLayout()
{
	RECT rc;
	::GetClientRect(GetParent(), &rc);

	SetPosition(0, 0);
	SetSize(rc.right, rc.bottom);

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

