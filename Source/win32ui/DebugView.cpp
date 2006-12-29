#include <boost/bind.hpp>
#include "DebugView.h"
#include "PtrMacro.h"

using namespace Framework;
using namespace boost;

CDebugView::CDebugView(HWND hParent, CMIPS* pCtx, const char* sName)
{
	m_pCtx				= pCtx;
	m_sName				= sName;

	m_pDisAsmWnd		= new CDisAsmWnd(hParent, m_pCtx);
	m_pRegViewWnd		= new CRegViewWnd(hParent, m_pCtx);
	m_pMemoryViewWnd	= new CMemoryViewMIPSWnd(hParent, m_pCtx);

	m_pCallStackWnd		= new CCallStackWnd(hParent, m_pCtx);
	m_pCallStackWnd->m_OnFunctionDblClick.connect(bind(&CDebugView::OnCallStackWndFunctionDblClick, this, _1));

	Hide();
}

CDebugView::~CDebugView()
{
	DELETEPTR(m_pDisAsmWnd);
	DELETEPTR(m_pRegViewWnd);
	DELETEPTR(m_pMemoryViewWnd);
	DELETEPTR(m_pCallStackWnd);
}

const char* CDebugView::GetName() const
{
	return m_sName;
}

void CDebugView::Hide()
{
	int nMethod;

	nMethod = SW_HIDE;

	m_pDisAsmWnd->Show(nMethod);
	m_pMemoryViewWnd->Show(nMethod);
	m_pRegViewWnd->Show(nMethod);
	m_pCallStackWnd->Show(nMethod);
}

CMIPS* CDebugView::GetContext()
{
	return m_pCtx;
}

CDisAsmWnd* CDebugView::GetDisassemblyWindow()
{
	return m_pDisAsmWnd;
}

CMemoryViewMIPSWnd* CDebugView::GetMemoryViewWindow()
{
	return m_pMemoryViewWnd;
}

CRegViewWnd* CDebugView::GetRegisterViewWindow()
{
	return m_pRegViewWnd;
}

CCallStackWnd* CDebugView::GetCallStackWindow()
{
	return m_pCallStackWnd;
}

void CDebugView::OnCallStackWndFunctionDblClick(uint32 nAddress)
{
	m_pDisAsmWnd->SetAddress(nAddress);
}
