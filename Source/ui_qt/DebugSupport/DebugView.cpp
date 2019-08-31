#include "DebugView.h"
#include "BiosDebugInfoProvider.h"
#include <QVBoxLayout>
//#include "DisAsmWnd.h"

CDebugView::CDebugView(QMdiArea* parent, CVirtualMachine& virtualMachine, CMIPS* ctx,
                       const StepFunction& stepFunction, CBiosDebugInfoProvider* biosDebugInfoProvider, const char* name)//, CDisAsmWnd::DISASM_TYPE disAsmType)
	  : m_virtualMachine(virtualMachine)
    , m_ctx(ctx)
    , m_name(name)
    , m_stepFunction(stepFunction)
		, m_mdiSubWindow(nullptr)
    //, m_disAsmWnd(nullptr)
    , m_regViewWnd(nullptr)
    //, m_memoryViewWnd(nullptr)
    //, m_callStackWnd(nullptr)
    , m_biosDebugInfoProvider(biosDebugInfoProvider)
{
	m_mdiSubWindow = new QMdiSubWindow(parent);
	this->m_mdiSubWindow->setMinimumHeight(750);
	this->m_mdiSubWindow->setMinimumWidth(320);
	parent->addSubWindow(m_mdiSubWindow);
	//m_mdiSubWindow->setWidget(m_viewTabs);
	
	// Setup tabs
	//m_disAsmWnd = new CDisAsmWnd(parentWnd, virtualMachine, m_ctx, disAsmType);
	//this->addTab(m_disAsmWnd, "DisAsm");
	m_regViewWnd = new CRegViewWnd(m_mdiSubWindow, this->m_ctx);
	this->m_mdiSubWindow->setWidget(m_regViewWnd);
	//m_memoryViewWnd = new CMemoryViewMIPSWnd(parentWnd, virtualMachine, m_ctx);
	//this->addTab(m_memoryViewWnd, "Memory View");

	//m_callStackWnd = new CCallStackWnd(parentWnd, m_ctx, m_biosDebugInfoProvider);
	//this->addTab(m_callStackWnd, "Call Stack");
	//m_OnFunctionDblClickConnection = m_callStackWnd->OnFunctionDblClick.Connect(std::bind(&CDebugView::OnCallStackWndFunctionDblClick, this, std::placeholders::_1));

	this->m_mdiSubWindow->show();
	//this->m_viewTabs->show();
	//Hide();
}

CDebugView::~CDebugView()
{
	//delete m_disAsmWnd;
	delete m_regViewWnd;
	//delete m_memoryViewWnd;
	//delete m_callStackWnd;
}

void CDebugView::HandleMachineStateChange()
{
	//m_disAsmWnd->HandleMachineStateChange();
	m_regViewWnd->HandleMachineStateChange();
	//m_memoryViewWnd->HandleMachineStateChange();
	//m_callStackWnd->HandleMachineStateChange();
}

void CDebugView::HandleRunningStateChange(CVirtualMachine::STATUS newState)
{
	//m_disAsmWnd->HandleRunningStateChange(newState);
	m_regViewWnd->HandleRunningStateChange(newState);
	//m_memoryViewWnd->HandleRunningStateChange(newState);
	//m_callStackWnd->HandleRunningStateChange(newState);
}

const char* CDebugView::GetName() const
{
	return m_name.c_str();
}

void CDebugView::Hide()
{
	//int method = SW_HIDE;
	//m_disAsmWnd->hide();
	//m_memoryViewWnd->hide();

	m_regViewWnd->hide();
	//m_callStackWnd->hide();
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

//CDisAsmWnd* CDebugView::GetDisassemblyWindow()
//{
	//return m_disAsmWnd;
//}

//CMemoryViewMIPSWnd* CDebugView::GetMemoryViewWindow()
//{
	//return m_memoryViewWnd;
//}

CRegViewWnd* CDebugView::GetRegisterViewWindow()
{
	return m_regViewWnd;
}

//CCallStackWnd* CDebugView::GetCallStackWindow()
//{
	//return m_callStackWnd;
//}

void CDebugView::OnCallStackWndFunctionDblClick(uint32 nAddress)
{
	//auto disAsm = m_disAsmWnd->GetDisAsm();
	//disAsm->SetCenterAtAddress(nAddress);
	//disAsm->SetSelectedAddress(nAddress);
}
