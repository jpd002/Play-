#include <stdio.h>
#include "ELFSectionView.h"
#include "PtrMacro.h"
#include "GridLayout.h"
#include "win32/Static.h"
#include "win32/LayoutWindow.h"

#define CLSNAME		_X("CELFSectionView")

using namespace Framework;

CELFSectionView::CELFSectionView(HWND hParent, CELF* pELF, uint16 nSection)
{
	RECT rc;
	CGridLayout* pSubLayout0;

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
	m_nSection = nSection;

	SetRect(&rc, 0, 0, 1, 1);

	Create(NULL, CLSNAME, _X(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, &rc, hParent, NULL);
	SetClassPtr();

	m_pType			= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pFlags		= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pAddress		= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pOffset		= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pSize			= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pLink			= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pInfo			= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pAlignment	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pEntrySize	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);

	m_pData = new CMemoryViewPtr(m_hWnd, &rc);

	pSubLayout0 = new CGridLayout(2, 9);

	pSubLayout0->SetObject(0, 0, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Type:"))));
	pSubLayout0->SetObject(0, 1, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Flags:"))));
	pSubLayout0->SetObject(0, 2, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Address:"))));
	pSubLayout0->SetObject(0, 3, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("File Offset:"))));
	pSubLayout0->SetObject(0, 4, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Size:"))));
	pSubLayout0->SetObject(0, 5, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Section Link:"))));
	pSubLayout0->SetObject(0, 6, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Info:"))));
	pSubLayout0->SetObject(0, 7, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Alignment:"))));
	pSubLayout0->SetObject(0, 8, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Entry Size:"))));

	pSubLayout0->SetObject(1, 0, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pType));
	pSubLayout0->SetObject(1, 1, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pFlags));
	pSubLayout0->SetObject(1, 2, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pAddress));
	pSubLayout0->SetObject(1, 3, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pOffset));
	pSubLayout0->SetObject(1, 4, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSize));
	pSubLayout0->SetObject(1, 5, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pLink));
	pSubLayout0->SetObject(1, 6, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pInfo));
	pSubLayout0->SetObject(1, 7, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pAlignment));
	pSubLayout0->SetObject(1, 8, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pEntrySize));

	pSubLayout0->SetVerticalStretch(0);

	m_pLayout = new CVerticalLayout;
	m_pLayout->InsertObject(pSubLayout0);
	m_pLayout->InsertObject(new CLayoutWindow(200, 200, 1, 1, m_pData));

	FillInformation();

	RefreshLayout();
}

CELFSectionView::~CELFSectionView()
{
	Destroy();
	DELETEPTR(m_pLayout);
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

void CELFSectionView::FillInformation()
{
	xchar sTemp[256];
	ELFSECTIONHEADER* pH;

	pH = m_pELF->GetSection(m_nSection);

	switch(pH->nType)
	{
	case 0x00:
		xstrcpy(sTemp, _X("SHT_NULL"));
		break;
	case 0x01:
		xstrcpy(sTemp, _X("SHT_PROGBITS"));
		break;
	case 0x02:
		xstrcpy(sTemp, _X("SHT_SYMTAB"));
		break;
	case 0x03:
		xstrcpy(sTemp, _X("SHT_STRTAB"));
		break;
	case 0x08:
		xstrcpy(sTemp, _X("SHT_NOBITS"));
		break;
	default:
		xsnprintf(sTemp, countof(sTemp), _X("Unknown (0x%0.8X)"), pH->nType);
		break;
	}
	m_pType->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nFlags);
	if(pH->nFlags & 0x7)
	{
		xstrcat(sTemp, _X(" ("));
		if(pH->nFlags & 0x01)
		{
			xstrcat(sTemp, _X("SHF_WRITE"));
			if(pH->nFlags & 0x6)
			{
				xstrcat(sTemp, _X(" | "));
			}
		}
		if(pH->nFlags & 0x2)
		{
			xstrcat(sTemp, _X("SHF_ALLOC"));
			if(pH->nFlags & 0x04)
			{
				xstrcat(sTemp, _X(" | "));
			}
		}
		if(pH->nFlags & 0x04)
		{
			xstrcat(sTemp, _X("SHF_EXECINSTR"));
		}
		xstrcat(sTemp, _X(")"));
	}
	m_pFlags->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nStart);
	m_pAddress->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nOffset);
	m_pOffset->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nSize);
	m_pSize->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("%i"), pH->nIndex);
	m_pLink->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("%i"), pH->nInfo);
	m_pInfo->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nAlignment);
	m_pAlignment->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nOther);
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

