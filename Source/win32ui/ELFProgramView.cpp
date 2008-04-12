#include <stdio.h>
#include "ELFProgramView.h"
#include "PtrMacro.h"
#include "layout/LayoutStretch.h"
#include "win32/Static.h"
#include "win32/LayoutWindow.h"

#define CLSNAME	_T("CELFProgramView")

using namespace Framework;

CELFProgramView::CELFProgramView(HWND hParent, CELF* pELF, uint16 nProgram)
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
	m_nProgram = nProgram;

	SetRect(&rc, 0, 0, 1, 1);

	Create(NULL, CLSNAME, _T(""), WS_CHILD | WS_DISABLED | WS_CLIPCHILDREN, &rc, hParent, NULL);
	SetClassPtr();

	m_pType		= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pOffset	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pVAddr	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pPAddr	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pFileSize	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pMemSize	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pFlags	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pAlign	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);

    m_pLayout = CGridLayout::Create(2, 9);

	m_pLayout->SetObject(0, 0, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Type:"))));
	m_pLayout->SetObject(0, 1, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Offset:"))));
	m_pLayout->SetObject(0, 2, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Virtual Address:"))));
	m_pLayout->SetObject(0, 3, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Physical Address:"))));
	m_pLayout->SetObject(0, 4, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("File Size:"))));
	m_pLayout->SetObject(0, 5, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Memory Size:"))));
	m_pLayout->SetObject(0, 6, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Flags:"))));
	m_pLayout->SetObject(0, 7, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Alignment:"))));

	m_pLayout->SetObject(1, 0, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pType));
	m_pLayout->SetObject(1, 1, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pOffset));
	m_pLayout->SetObject(1, 2, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pVAddr));
	m_pLayout->SetObject(1, 3, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pPAddr));
	m_pLayout->SetObject(1, 4, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pFileSize));
	m_pLayout->SetObject(1, 5, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pMemSize));
	m_pLayout->SetObject(1, 6, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pFlags));
	m_pLayout->SetObject(1, 7, Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pAlign));
    m_pLayout->SetObject(1, 8, CLayoutStretch::Create());

	FillInformation();

	RefreshLayout();

}

CELFProgramView::~CELFProgramView()
{
    Destroy();
}

long CELFProgramView::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

void CELFProgramView::FillInformation()
{
	ELFPROGRAMHEADER* pH;
	TCHAR sTemp[256];
	
	pH = m_pELF->GetProgram(m_nProgram);

	switch(pH->nType)
	{
	case 0x00:
		_tcscpy(sTemp, _T("PT_NULL"));
		break;
	case 0x01:
		_tcscpy(sTemp, _T("PT_LOAD"));
		break;
	case 0x02:
		_tcscpy(sTemp, _T("PT_DYNAMIC"));
		break;
	case 0x03:
		_tcscpy(sTemp, _T("PT_INTERP"));
		break;
	case 0x04:
		_tcscpy(sTemp, _T("PT_NOTE"));
		break;
	case 0x05:
		_tcscpy(sTemp, _T("PT_SHLIB"));
		break;
	case 0x06:
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
