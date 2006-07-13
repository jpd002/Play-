#include <stdio.h>
#include "ELFHeaderView.h"
#include "PtrMacro.h"
#include "LayoutStretch.h"
#include "win32/Static.h"
#include "win32/LayoutWindow.h"

#define CLSNAME _X("CELFHeaderView")

using namespace Framework;

CELFHeaderView::CELFHeaderView(HWND hParent, CELF* pELF)
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

	Create(NULL, CLSNAME, _X(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, &rc, hParent, NULL);
	SetClassPtr();
	
	m_pType		= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pMachine	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pVersion	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pEntry	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pPHOffset	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pPHSize	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pPHCount	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pSHOffset	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pSHSize	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pSHCount	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pFlags	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pSize		= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);
	m_pSHStrTab = new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY);

	m_pLayout = new CGridLayout(2, 14);

	m_pLayout->SetObject(0,  0, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Type:"))));
	m_pLayout->SetObject(0,  1, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Machine:"))));
	m_pLayout->SetObject(0,  2, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Version:"))));
	m_pLayout->SetObject(0,  3, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Entry Point:"))));
	m_pLayout->SetObject(0,  4, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Flags:"))));
	m_pLayout->SetObject(0,  5, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Header Size:"))));
	m_pLayout->SetObject(0,  6, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Program Header Table Offset:"))));
	m_pLayout->SetObject(0,  7, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Program Header Size:"))));
	m_pLayout->SetObject(0,  8, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Program Header Count:"))));
	m_pLayout->SetObject(0,  9, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Section Header Table Offset:"))));
	m_pLayout->SetObject(0, 10, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Section Header Size:"))));
	m_pLayout->SetObject(0, 11, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Section Header Count:"))));
	m_pLayout->SetObject(0, 12, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Section Header String Table Index:"))));

	m_pLayout->SetObject(1,  0, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pType));
	m_pLayout->SetObject(1,  1, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pMachine));
	m_pLayout->SetObject(1,  2, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pVersion));
	m_pLayout->SetObject(1,  3, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pEntry));
	m_pLayout->SetObject(1,  4, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pFlags));
	m_pLayout->SetObject(1,  5, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSize));
	m_pLayout->SetObject(1,  6, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pPHOffset));
	m_pLayout->SetObject(1,  7, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pPHSize));
	m_pLayout->SetObject(1,  8, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pPHCount));
	m_pLayout->SetObject(1,  9, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHOffset));
	m_pLayout->SetObject(1, 10, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHSize));
	m_pLayout->SetObject(1, 11, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHCount));
	m_pLayout->SetObject(1, 12, CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHStrTab));
	m_pLayout->SetObject(1, 13, new CLayoutStretch);

	FillInformation();
	
	RefreshLayout();
}

CELFHeaderView::~CELFHeaderView()
{
	Destroy();
	DELETEPTR(m_pLayout);
}

void CELFHeaderView::FillInformation()
{
	xchar sTemp[256];
	ELFHEADER* pH;

	pH = &m_pELF->m_Header;

	switch(pH->nType)
	{
	case 0x00:
		xstrcpy(sTemp, _X("ET_NONE"));
		break;
	case 0x01:
		xstrcpy(sTemp, _X("ET_REL"));
		break;
	case 0x02:
		xstrcpy(sTemp, _X("ET_EXEC"));
		break;
	case 0x03:
		xstrcpy(sTemp, _X("ET_DYN"));
		break;
	case 0x04:
		xstrcpy(sTemp, _X("ET_CORE"));
		break;
	default:
		xsnprintf(sTemp, countof(sTemp), _X("Unknown (%i)"), pH->nType);
		break;
	}
	m_pType->SetText(sTemp);

	switch(pH->nCPU)
	{
	case 0x00:
		xstrcpy(sTemp, _X("EM_NONE"));
		break;
	case 0x01:
		xstrcpy(sTemp, _X("EM_M32"));
		break;
	case 0x02:
		xstrcpy(sTemp, _X("EM_SPARC"));
		break;
	case 0x03:
		xstrcpy(sTemp, _X("EM_386"));
		break;
	case 0x04:
		xstrcpy(sTemp, _X("EM_68K"));
		break;
	case 0x05:
		xstrcpy(sTemp, _X("EM_88K"));
		break;
	case 0x07:
		xstrcpy(sTemp, _X("EM_860"));
		break;
	case 0x08:
		xstrcpy(sTemp, _X("EM_MIPS"));
		break;
	default:
		xsnprintf(sTemp, countof(sTemp), _X("Unknown (%i)"), pH->nCPU);
		break;
	}
	m_pMachine->SetText(sTemp);

	switch(pH->nVersion)
	{
	case 0x00:
		xstrcpy(sTemp, _X("EV_NONE"));
		break;
	case 0x01:
		xstrcpy(sTemp, _X("EV_CURRENT"));
		break;
	default:
		xsnprintf(sTemp, countof(sTemp), _X("Unknown (%i)"), pH->nVersion);
		break;
	}
	m_pVersion->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nEntryPoint);
	m_pEntry->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nProgHeaderStart);
	m_pPHOffset->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nProgHeaderEntrySize);
	m_pPHSize->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("%i"), pH->nProgHeaderCount);
	m_pPHCount->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nSectHeaderStart);
	m_pSHOffset->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nSectHeaderEntrySize);
	m_pSHSize->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("%i"), pH->nSectHeaderCount);
	m_pSHCount->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nFlags);
	m_pFlags->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("0x%0.8X"), pH->nSize);
	m_pSize->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("%i"), pH->nSectHeaderStringTableIndex);
	m_pSHStrTab->SetText(sTemp);
}

void CELFHeaderView::RefreshLayout()
{
	RECT rc;
	::GetClientRect(GetParent(), &rc);

	SetPosition(0, 0);
	SetSize(rc.right, rc.bottom);

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom + m_pLayout->GetPreferredHeight());

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

long CELFHeaderView::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}
