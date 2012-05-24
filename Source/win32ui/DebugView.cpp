#include "DebugView.h"

CDebugView::CDebugView(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* pCtx, 
	const StepFunction& stepFunction, CBiosDebugInfoProvider* biosDebugInfoProvider, const char* sName) 
: m_virtualMachine(virtualMachine)
, m_pCtx(pCtx)
, m_name(sName)
, m_stepFunction(stepFunction)
, m_pDisAsmWnd(nullptr)
, m_pRegViewWnd(nullptr)
, m_pMemoryViewWnd(nullptr)
, m_pCallStackWnd(nullptr)
, m_biosDebugInfoProvider(biosDebugInfoProvider)
{
	m_pDisAsmWnd		= new CDisAsmWnd(hParent, virtualMachine, m_pCtx);
	m_pRegViewWnd		= new CRegViewWnd(hParent, virtualMachine, m_pCtx);
	m_pMemoryViewWnd	= new CMemoryViewMIPSWnd(hParent, virtualMachine, m_pCtx);

	m_pCallStackWnd		= new CCallStackWnd(hParent, virtualMachine, m_pCtx, m_biosDebugInfoProvider);
	m_pCallStackWnd->OnFunctionDblClick.connect(boost::bind(&CDebugView::OnCallStackWndFunctionDblClick, this, _1));

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

CBiosDebugInfoProvider* CDebugView::GetBiosDebugInfoProvider() const
{
	return m_biosDebugInfoProvider;
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
	m_pDisAsmWnd->SetCenterAtAddress(nAddress);
	m_pDisAsmWnd->SetSelectedAddress(nAddress);
}
