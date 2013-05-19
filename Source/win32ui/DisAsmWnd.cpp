#include "DisAsmWnd.h"
#include "PtrMacro.h"

#define WNDSTYLE (WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CDisAsmWnd::CDisAsmWnd(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* pCtx)
{
	Create(NULL, Framework::Win32::CDefaultWndClass::GetName(), _T("Disassembly"), WNDSTYLE, 
		Framework::Win32::CRect(0, 0, 320, 240), hParent, NULL);

	SetClassPtr();

	m_pDisAsm = new CDisAsm(m_hWnd, Framework::Win32::CRect(0, 0, 320, 240), virtualMachine, pCtx);

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

void CDisAsmWnd::SetCenterAtAddress(uint32 address)
{
	m_pDisAsm->SetCenterAtAddress(address);
}

void CDisAsmWnd::SetSelectedAddress(uint32 address)
{
	m_pDisAsm->SetSelectedAddress(address);
}

void CDisAsmWnd::Refresh()
{
	m_pDisAsm->Redraw();
}

void CDisAsmWnd::RefreshLayout()
{
	RECT rc = GetClientRect();
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
