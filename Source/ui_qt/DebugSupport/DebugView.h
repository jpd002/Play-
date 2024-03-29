#pragma once

#include <string>
#include <functional>
#include <QWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include "DisAsmWnd.h"
#include "MemoryViewMIPSWnd.h"
#include "RegViewWnd.h"
#include "CallStackWnd.h"
#include "VirtualMachineStateView.h"

class CBiosDebugInfoProvider;

class CDebugView : public CVirtualMachineStateView
{
public:
	typedef std::function<void(void)> StepFunction;

	CDebugView(QWidget*, QMdiArea*, CVirtualMachine&, CMIPS*, const StepFunction&, CBiosDebugInfoProvider*, const char*, uint64, CDisAsmTableModel::DISASM_TYPE = CDisAsmTableModel::DISASM_STANDARD);
	virtual ~CDebugView();

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

	CMIPS* GetContext();
	CDisAsmWnd* GetDisassemblyWindow();
	CMemoryViewMIPSWnd* GetMemoryViewWindow();
	CRegViewWnd* GetRegisterViewWindow();
	CCallStackWnd* GetCallStackWindow();

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
	QMdiSubWindow* m_disAsmWnd = nullptr;
	QMdiSubWindow* m_memoryViewWnd = nullptr;
	QMdiSubWindow* m_regViewWnd = nullptr;
	QMdiSubWindow* m_callStackWnd = nullptr;
	StepFunction m_stepFunction;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider;

	CCallStackWnd::OnFunctionDblClickSignal::Connection m_OnFunctionDblClickConnection;
};
