#include "DisAsmWnd.h"
#include "DisAsmVu.h"

#define WNDSTYLE (WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CDisAsmWnd::CDisAsmWnd(QMdiArea* parent, CVirtualMachine& virtualMachine, CMIPS* ctx, DISASM_TYPE disAsmType)
    : QMdiSubWindow(parent)
{

	resize(320, 240);

	parent->addSubWindow(this);
	setWindowTitle("Disassembly");

	switch(disAsmType)
	{
	case DISASM_STANDARD:
		m_disAsm = new CDisAsm(this, virtualMachine, ctx);
		break;
	case DISASM_VU:
		m_disAsm = new CDisAsmVu(this, virtualMachine, ctx);
		break;
	default:
		assert(0);
		break;
	}

	// RefreshLayout();
}

CDisAsmWnd::~CDisAsmWnd()
{
	delete m_disAsm;
}

CDisAsm* CDisAsmWnd::GetDisAsm() const
{
	return m_disAsm;
}

void CDisAsmWnd::HandleMachineStateChange()
{
	m_disAsm->HandleMachineStateChange();
}

void CDisAsmWnd::HandleRunningStateChange(CVirtualMachine::STATUS newState)
{
	m_disAsm->HandleRunningStateChange(newState);
}

// void CDisAsmWnd::RefreshLayout()
// {
// 	RECT rc = GetClientRect();
// 	m_disAsm->SetSize(rc.right, rc.bottom);
// }

// long CDisAsmWnd::OnSetFocus()
// {
// 	m_disAsm->SetFocus();
// 	return FALSE;
// }

// long CDisAsmWnd::OnSize(unsigned int nType, unsigned int nCX, unsigned int nCY)
// {
// 	RefreshLayout();
// 	return TRUE;
// }

// long CDisAsmWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
// {
// 	switch(nCmd)
// 	{
// 	case SC_CLOSE:
// 		Show(SW_HIDE);
// 		return FALSE;
// 	}
// 	return TRUE;
// }
