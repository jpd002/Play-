#include "DebugView.h"

CDebugView::CDebugView(HWND parentWnd, CVirtualMachine& virtualMachine, CMIPS* ctx,
                       const StepFunction& stepFunction, CBiosDebugInfoProvider* biosDebugInfoProvider, const char* name, CDisAsmWnd::DISASM_TYPE disAsmType)
    : m_virtualMachine(virtualMachine)
    , m_ctx(ctx)
    , m_name(name)
    , m_stepFunction(stepFunction)
    , m_disAsmWnd(nullptr)
    , m_regViewWnd(nullptr)
    , m_memoryViewWnd(nullptr)
    , m_callStackWnd(nullptr)
    , m_biosDebugInfoProvider(biosDebugInfoProvider)
{
	m_disAsmWnd = new CDisAsmWnd(parentWnd, virtualMachine, m_ctx, disAsmType);
	m_regViewWnd = new CRegViewWnd(parentWnd, m_ctx);
	m_memoryViewWnd = new CMemoryViewMIPSWnd(parentWnd, virtualMachine, m_ctx);

	m_callStackWnd = new CCallStackWnd(parentWnd, m_ctx, m_biosDebugInfoProvider);
	m_callStackWnd->OnFunctionDblClick.connect(boost::bind(&CDebugView::OnCallStackWndFunctionDblClick, this, _1));

	Hide();
}

CDebugView::~CDebugView()
{
	delete m_disAsmWnd;
	delete m_regViewWnd;
	delete m_memoryViewWnd;
	delete m_callStackWnd;
}

void CDebugView::HandleMachineStateChange()
{
	m_disAsmWnd->HandleMachineStateChange();
	m_regViewWnd->HandleMachineStateChange();
	m_memoryViewWnd->HandleMachineStateChange();
	m_callStackWnd->HandleMachineStateChange();
}

void CDebugView::HandleRunningStateChange(CVirtualMachine::STATUS newState)
{
	m_disAsmWnd->HandleRunningStateChange(newState);
	m_regViewWnd->HandleRunningStateChange(newState);
	m_memoryViewWnd->HandleRunningStateChange(newState);
	m_callStackWnd->HandleRunningStateChange(newState);
}

const char* CDebugView::GetName() const
{
	return m_name.c_str();
}

void CDebugView::Hide()
{
	int method = SW_HIDE;
	m_disAsmWnd->Show(method);
	m_memoryViewWnd->Show(method);
	m_regViewWnd->Show(method);
	m_callStackWnd->Show(method);
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
	return m_ctx;
}

CDisAsmWnd* CDebugView::GetDisassemblyWindow()
{
	return m_disAsmWnd;
}

CMemoryViewMIPSWnd* CDebugView::GetMemoryViewWindow()
{
	return m_memoryViewWnd;
}

CRegViewWnd* CDebugView::GetRegisterViewWindow()
{
	return m_regViewWnd;
}

CCallStackWnd* CDebugView::GetCallStackWindow()
{
	return m_callStackWnd;
}

void CDebugView::OnCallStackWndFunctionDblClick(uint32 nAddress)
{
	auto disAsm = m_disAsmWnd->GetDisAsm();
	disAsm->SetCenterAtAddress(nAddress);
	disAsm->SetSelectedAddress(nAddress);
}
