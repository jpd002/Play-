#include "DebugView.h"
#include "BiosDebugInfoProvider.h"
#include <QVBoxLayout>

CDebugView::CDebugView(QWidget* parent, QMdiArea* mdiArea, CVirtualMachine& virtualMachine, CMIPS* ctx,
                       const StepFunction& stepFunction, CBiosDebugInfoProvider* biosDebugInfoProvider, const char* name, int size, CQtDisAsmTableModel::DISASM_TYPE disAsmType)
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

	auto disAsm = new CDisAsmWnd(parent, virtualMachine, m_ctx, name, disAsmType);
	m_disAsmWnd = new QMdiSubWindow(mdiArea);
	m_disAsmWnd->setWidget(disAsm);
	m_disAsmWnd->setWindowTitle("Disassembly");

	auto regViewWnd = new CRegViewWnd(parent, m_ctx);
	m_regViewWnd = new QMdiSubWindow(mdiArea);
	m_regViewWnd->setWidget(regViewWnd);
	m_regViewWnd->setWindowTitle("Registers");

	auto memoryViewWnd = new CMemoryViewMIPSWnd(parent, virtualMachine, m_ctx, size);
	m_memoryViewWnd = new QMdiSubWindow(mdiArea);
	m_memoryViewWnd->setWidget(memoryViewWnd);
	m_memoryViewWnd->setWindowTitle("Memory View");

	auto callStackWnd = new CCallStackWnd(parent, m_ctx, m_biosDebugInfoProvider);
	m_callStackWnd = new QMdiSubWindow(mdiArea);
	m_callStackWnd->setWidget(callStackWnd);
	m_callStackWnd->setWindowTitle("Call Stack");

	m_OnFunctionDblClickConnection = callStackWnd->OnFunctionDblClick.Connect(std::bind(&CDebugView::OnCallStackWndFunctionDblClick, this, std::placeholders::_1));

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
	GetDisassemblyWindow()->HandleMachineStateChange();
	GetRegisterViewWindow()->HandleMachineStateChange();
	GetMemoryViewWindow()->HandleMachineStateChange();
	GetCallStackWindow()->HandleMachineStateChange();
}

void CDebugView::HandleRunningStateChange(CVirtualMachine::STATUS newState)
{
	GetDisassemblyWindow()->HandleRunningStateChange(newState);
	GetRegisterViewWindow()->HandleRunningStateChange(newState);
	GetMemoryViewWindow()->HandleRunningStateChange(newState);
	GetCallStackWindow()->HandleRunningStateChange(newState);
}

const char* CDebugView::GetName() const
{
	return m_name.c_str();
}

void CDebugView::Hide()
{
	m_disAsmWnd->hide();
	m_memoryViewWnd->hide();

	m_regViewWnd->hide();
	m_callStackWnd->hide();
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
	return static_cast<CDisAsmWnd*>(m_disAsmWnd->widget());
}

CMemoryViewMIPSWnd* CDebugView::GetMemoryViewWindow()
{
	return static_cast<CMemoryViewMIPSWnd*>(m_memoryViewWnd->widget());
}

CRegViewWnd* CDebugView::GetRegisterViewWindow()
{
	return static_cast<CRegViewWnd*>(m_regViewWnd->widget());
}

CCallStackWnd* CDebugView::GetCallStackWindow()
{
	return static_cast<CCallStackWnd*>(m_callStackWnd->widget());
}

void CDebugView::OnCallStackWndFunctionDblClick(uint32 nAddress)
{
	auto disAsm = GetDisassemblyWindow();
	disAsm->SetCenterAtAddress(nAddress);
	disAsm->SetSelectedAddress(nAddress);
}
