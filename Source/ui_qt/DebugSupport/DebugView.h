#pragma once

#include <string>
#include <functional>
#include <QWidget>
#include <QTabWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
//#include "DisAsmWnd.h"
//#include "MemoryViewMIPSWnd.h"
#include "RegViewWnd.h"
//#include "CallStackWnd.h"
#include "VirtualMachineStateView.h"

class CBiosDebugInfoProvider;

class CDebugView : public CVirtualMachineStateView
{
public:
		typedef std::function<void(void)> StepFunction;

	CDebugView(QMdiArea*, CVirtualMachine&, CMIPS*, const StepFunction&, CBiosDebugInfoProvider*, const char*);//, CDisAsmWnd::DISASM_TYPE = CDisAsmWnd::DISASM_STANDARD);
	virtual ~CDebugView();

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

	CMIPS* GetContext();
	//CDisAsmWnd* GetDisassemblyWindow();
	//CMemoryViewMIPSWnd* GetMemoryViewWindow();
	CRegViewWnd* GetRegisterViewWindow();
	//CCallStackWnd* GetCallStackWindow();

	void Step();
	const char* GetName() const;
	void Hide();
	CBiosDebugInfoProvider* GetBiosDebugInfoProvider() const;

protected:
	void OnCallStackWndFunctionDblClick(uint32_t);

private:
	std::string m_name;

	CVirtualMachine& m_virtualMachine;
	CMIPS* m_ctx;
	//CDisAsmWnd* m_disAsmWnd;
	//CMemoryViewMIPSWnd* m_memoryViewWnd;
	CRegViewWnd* m_regViewWnd;
	//CCallStackWnd* m_callStackWnd;
	StepFunction m_stepFunction;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider;

	//CCallStackWnd::OnFunctionDblClickSignal::Connection m_OnFunctionDblClickConnection;
	QMdiSubWindow* m_mdiSubWindow;
	QTabWidget* m_viewTabs;
};
