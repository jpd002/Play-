#include "DisAsmWnd.h"
#include "DisAsmVu.h"

#define WNDSTYLE (WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CDisAsmWnd::CDisAsmWnd(HWND parentWnd, CVirtualMachine& virtualMachine, CMIPS* ctx, DISASM_TYPE disAsmType)
{
	Create(NULL, Framework::Win32::CDefaultWndClass::GetName(), _T("Disassembly"), WNDSTYLE, 
		Framework::Win32::CRect(0, 0, 320, 240), parentWnd, NULL);

	SetClassPtr();

	switch(disAsmType)
	{
	case DISASM_STANDARD:
		m_disAsm = new CDisAsm(m_hWnd, Framework::Win32::CRect(0, 0, 320, 240), virtualMachine, ctx);
		break;
	case DISASM_VU:
		m_disAsm = new CDisAsmVu(m_hWnd, Framework::Win32::CRect(0, 0, 320, 240), virtualMachine, ctx);
		break;
	default:
		assert(0);
		break;
	}

	RefreshLayout();
}

CDisAsmWnd::~CDisAsmWnd()
{
	delete m_disAsm;
}

CDisAsm* CDisAsmWnd::GetDisAsm() const
{
	return m_disAsm;
}

void CDisAsmWnd::Refresh()
{
	m_disAsm->Redraw();
}

void CDisAsmWnd::RefreshLayout()
{
	RECT rc = GetClientRect();
	m_disAsm->SetSize(rc.right, rc.bottom);
}

long CDisAsmWnd::OnSetFocus()
{
	m_disAsm->SetFocus();
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
