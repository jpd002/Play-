#include "DisAsmWnd.h"
#include "PtrMacro.h"

#define CLSNAME		_T("CDisAsmWnd")

using namespace Framework;

CDisAsmWnd::CDisAsmWnd(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* pCtx)
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

	Create(NULL, CLSNAME, _T("Disassembly"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, &rc, hParent, NULL);
	SetClassPtr();

	m_pDisAsm = new CDisAsm(m_hWnd, &rc, virtualMachine, pCtx);

	RefreshLayout();
}

CDisAsmWnd::~CDisAsmWnd()
{
	DELETEPTR(m_pDisAsm);
}

void CDisAsmWnd::SetAddress(uint32 nAddress)
{
	m_pDisAsm->SetAddress(nAddress);
}

void CDisAsmWnd::Refresh()
{
	m_pDisAsm->Redraw();
}

void CDisAsmWnd::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

	m_pDisAsm->SetSize(rc.right, rc.bottom);
}

long CDisAsmWnd::OnSetFocus()
{
	m_pDisAsm->SetFocus();
	return FALSE;
}

long CDisAsmWnd::OnSize(unsigned int nType, unsigned int nCX, unsigned int nCY)
{
	RefreshLayout();
	return TRUE;
}

long CDisAsmWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

long CDisAsmWnd::OnCopy()
{
    return static_cast<long>(SendMessage(m_pDisAsm->m_hWnd, WM_COPY, 0, 0));
}
