#include "DebugView.h"

CDebugView::CDebugView(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* pCtx, const StepFunction& stepFunction, const char* sName) 
: m_virtualMachine(virtualMachine)
, m_pCtx(pCtx)
, m_name(sName)
, m_stepFunction(stepFunction)
, m_pDisAsmWnd(NULL)
, m_pRegViewWnd(NULL)
, m_pMemoryViewWnd(NULL)
, m_pCallStackWnd(NULL)
{
	m_pDisAsmWnd		= new CDisAsmWnd(hParent, virtualMachine, m_pCtx);
	m_pRegViewWnd		= new CRegViewWnd(hParent, virtualMachine, m_pCtx);
	m_pMemoryViewWnd	= new CMemoryViewMIPSWnd(hParent, virtualMachine, m_pCtx);

	m_pCallStackWnd		= new CCallStackWnd(hParent, virtualMachine, m_pCtx);
	m_pCallStackWnd->m_OnFunctionDblClick.connect(boost::bind(&CDebugView::OnCallStackWndFunctionDblClick, this, _1));

	Hide();
}

CDebugView::~CDebugView()
{
	delete m_pDisAsmWnd;
	delete m_pRegViewWnd;
	delete m_pMemoryViewWnd;
	delete m_pCallStackWnd;
}

const char* CDebugView::GetName() const
{
	return m_name.c_str();
}

void CDebugView::Hide()
{
	int nMethod = SW_HIDE;

	m_pDisAsmWnd->Show(nMethod);
	m_pMemoryViewWnd->Show(nMethod);
	m_pRegViewWnd->Show(nMethod);
	m_pCallStackWnd->Show(nMethod);
}

void CDebugView::Step()
{
	m_stepFunction();
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
