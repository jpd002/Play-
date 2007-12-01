#include "MemoryViewMIPSWnd.h"
#include "PtrMacro.h"
#include "lexical_cast_ex.h"
#include <boost/bind.hpp>

#define CLSNAME		_T("MemoryViewMIPSWnd")

using namespace Framework;
using namespace std;
using namespace boost;

CMemoryViewMIPSWnd::CMemoryViewMIPSWnd(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* pCtx)
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
	
	SetRect(&rc, 0, 0, 320, 240);

	Create(NULL, CLSNAME, _T("Memory"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, &rc, hParent, NULL);
	SetClassPtr();

    m_pStatusBar = new Win32::CStatusBar(m_hWnd);

    m_pMemoryView = new CMemoryViewMIPS(m_hWnd, &rc, virtualMachine, pCtx);
    m_pMemoryView->m_OnSelectionChange.connect(bind(&CMemoryViewMIPSWnd::OnMemoryViewSelectionChange, this, _1));

    UpdateStatusBar();
	RefreshLayout();
}

CMemoryViewMIPSWnd::~CMemoryViewMIPSWnd()
{
    DELETEPTR(m_pStatusBar);
    DELETEPTR(m_pMemoryView);
}

long CMemoryViewMIPSWnd::OnSize(unsigned int nType, unsigned int nCX, unsigned int nCY)
{
	RefreshLayout();
	return TRUE;
}

long CMemoryViewMIPSWnd::OnSetFocus()
{
	m_pMemoryView->SetFocus();
	return FALSE;
}

long CMemoryViewMIPSWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CMemoryViewMIPSWnd::UpdateStatusBar()
{
    tstring sCaption;
    sCaption = _T("Address : 0x") + lexical_cast_hex<tstring>(m_pMemoryView->GetSelection(), 8);
    m_pStatusBar->SetText(0, sCaption.c_str());
}

void CMemoryViewMIPSWnd::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

    m_pStatusBar->RefreshGeometry();
	m_pMemoryView->SetSize(rc.right, rc.bottom - m_pStatusBar->GetHeight());
}

void CMemoryViewMIPSWnd::OnMemoryViewSelectionChange(uint32 nAddress)
{
    UpdateStatusBar();
}
