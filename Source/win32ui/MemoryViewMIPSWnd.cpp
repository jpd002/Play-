#include "MemoryViewMIPSWnd.h"
#include "PtrMacro.h"

#define CLSNAME		_T("MemoryViewMIPSWnd")

using namespace Framework;

CMemoryViewMIPSWnd::CMemoryViewMIPSWnd(HWND hParent, CMIPS* pCtx)
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

	m_pMemoryView = new CMemoryViewMIPS(m_hWnd, &rc, pCtx);

	RefreshLayout();
}

CMemoryViewMIPSWnd::~CMemoryViewMIPSWnd()
{
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

void CMemoryViewMIPSWnd::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

	m_pMemoryView->SetSize(rc.right, rc.bottom);
}
