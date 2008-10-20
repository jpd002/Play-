#include <stdio.h>
#include "ELFHeaderView.h"
#include "PtrMacro.h"
#include "layout/LayoutStretch.h"
#include "win32/Static.h"
#include "win32/LayoutWindow.h"
#include <boost/lexical_cast.hpp>
#include "lexical_cast_ex.h"

#define CLSNAME _T("CELFHeaderView")

using namespace Framework;
using namespace boost;
using namespace std;

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

	Create(NULL, CLSNAME, _T(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, &rc, hParent, NULL);
	SetClassPtr();
	
	m_pType		= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pMachine	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pVersion	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pEntry	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pPHOffset	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pPHSize	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pPHCount	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pSHOffset	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pSHSize	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pSHCount	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pFlags	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pSize		= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pSHStrTab = new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);

    m_pLayout = CGridLayout::Create(2, 14);

	m_pLayout->SetObject(0,  0, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Type:"))));
	m_pLayout->SetObject(0,  1, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Machine:"))));
	m_pLayout->SetObject(0,  2, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Version:"))));
	m_pLayout->SetObject(0,  3, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Entry Point:"))));
	m_pLayout->SetObject(0,  4, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Flags:"))));
	m_pLayout->SetObject(0,  5, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Header Size:"))));
	m_pLayout->SetObject(0,  6, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Program Header Table Offset:"))));
	m_pLayout->SetObject(0,  7, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Program Header Size:"))));
	m_pLayout->SetObject(0,  8, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Program Header Count:"))));
	m_pLayout->SetObject(0,  9, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Section Header Table Offset:"))));
	m_pLayout->SetObject(0, 10, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Section Header Size:"))));
	m_pLayout->SetObject(0, 11, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Section Header Count:"))));
	m_pLayout->SetObject(0, 12, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Section Header String Table Index:"))));

	m_pLayout->SetObject(1,  0, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pType));
	m_pLayout->SetObject(1,  1, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pMachine));
	m_pLayout->SetObject(1,  2, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pVersion));
	m_pLayout->SetObject(1,  3, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pEntry));
	m_pLayout->SetObject(1,  4, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pFlags));
	m_pLayout->SetObject(1,  5, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSize));
	m_pLayout->SetObject(1,  6, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pPHOffset));
	m_pLayout->SetObject(1,  7, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pPHSize));
	m_pLayout->SetObject(1,  8, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pPHCount));
	m_pLayout->SetObject(1,  9, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHOffset));
	m_pLayout->SetObject(1, 10, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHSize));
	m_pLayout->SetObject(1, 11, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHCount));
	m_pLayout->SetObject(1, 12, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pSHStrTab));
    m_pLayout->SetObject(1, 13, CLayoutStretch::Create());

	FillInformation();
	
	RefreshLayout();
}

CELFHeaderView::~CELFHeaderView()
{
	Destroy();
}

void CELFHeaderView::FillInformation()
{
	TCHAR sTemp[256];
	const ELFHEADER* pH = &m_pELF->GetHeader();

	switch(pH->nType)
	{
	case 0x00:
		_tcscpy(sTemp, _T("ET_NONE"));
		break;
	case 0x01:
		_tcscpy(sTemp, _T("ET_REL"));
		break;
	case 0x02:
		_tcscpy(sTemp, _T("ET_EXEC"));
		break;
	case 0x03:
		_tcscpy(sTemp, _T("ET_DYN"));
		break;
	case 0x04:
		_tcscpy(sTemp, _T("ET_CORE"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (%i)"), pH->nType);
		break;
	}
	m_pType->SetText(sTemp);

	switch(pH->nCPU)
	{
	case 0x00:
		_tcscpy(sTemp, _T("EM_NONE"));
		break;
	case 0x01:
		_tcscpy(sTemp, _T("EM_M32"));
		break;
	case 0x02:
		_tcscpy(sTemp, _T("EM_SPARC"));
		break;
	case 0x03:
		_tcscpy(sTemp, _T("EM_386"));
		break;
	case 0x04:
		_tcscpy(sTemp, _T("EM_68K"));
		break;
	case 0x05:
		_tcscpy(sTemp, _T("EM_88K"));
		break;
	case 0x07:
		_tcscpy(sTemp, _T("EM_860"));
		break;
	case 0x08:
		_tcscpy(sTemp, _T("EM_MIPS"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (%i)"), pH->nCPU);
		break;
	}
	m_pMachine->SetText(sTemp);

	switch(pH->nVersion)
	{
	case 0x00:
		_tcscpy(sTemp, _T("EV_NONE"));
		break;
	case 0x01:
		_tcscpy(sTemp, _T("EV_CURRENT"));
		break;
	default:
		_sntprintf(sTemp, countof(sTemp), _T("Unknown (%i)"), pH->nVersion);
		break;
	}
	m_pVersion->SetText(sTemp);

	m_pEntry->SetText((_T("0x") + lexical_cast_hex<tstring>(pH->nEntryPoint, 8)).c_str());
	m_pPHOffset->SetText((_T("0x") + lexical_cast_hex<tstring>(pH->nProgHeaderStart, 8)).c_str());
	m_pPHSize->SetText((_T("0x") + lexical_cast_hex<tstring>(pH->nProgHeaderEntrySize, 8)).c_str());
	m_pPHCount->SetText(lexical_cast<tstring>(pH->nProgHeaderCount).c_str());
	m_pSHOffset->SetText((_T("0x") + lexical_cast_hex<tstring>(pH->nSectHeaderStart, 8)).c_str());
	m_pSHSize->SetText((_T("0x") + lexical_cast_hex<tstring>(pH->nSectHeaderEntrySize, 8)).c_str());
	m_pSHCount->SetText(lexical_cast<tstring>(pH->nSectHeaderCount).c_str());
	m_pFlags->SetText((_T("0x") + lexical_cast_hex<tstring>(pH->nFlags, 8)).c_str());
	m_pSize->SetText((_T("0x") + lexical_cast_hex<tstring>(pH->nSize, 8)).c_str());
	m_pSHStrTab->SetText(lexical_cast<tstring>(pH->nSectHeaderStringTableIndex).c_str());
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
