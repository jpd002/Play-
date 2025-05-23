#pragma once

#include <QMainWindow>
#include "VirtualMachine.h"
#include "FunctionsView.h"
#include "AddressListViewWnd.h"
#include "DisAsmWnd.h"

namespace Ui
{
	class DebuggerWindow;
}

class CPsfVm;
class CDebugView;

class DebuggerWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit DebuggerWindow(CPsfVm&, QWidget* parent = 0);
	~DebuggerWindow();

	void Reset();

private slots:
	void on_actionStep_CPU_triggered();
	void on_actionfind_word_value_triggered();
	void on_actionFind_Word_Half_Value_triggered();

signals:
	void OnMachineStateChange();
	void OnRunningStateChange();

private:
	void Layout1600x1200();

	void FindWordValue(uint32);

	//Event handlers
	void OnFunctionsViewFunctionDblClick(uint32);
	void OnFunctionsViewFunctionsStateChange();
	void OnMachineStateChangeMsg();
	void OnRunningStateChangeMsg();
	void OnFindCallersRequested(uint32);
	void OnFindCallersAddressDblClick(uint32);

	CAddressListViewWnd::AddressSelectedEvent::Connection m_addressSelectedConnection;

	Framework::CSignal<void(uint32)>::Connection m_OnFunctionDblClickConnection;
	Framework::CSignal<void()>::Connection m_OnFunctionsStateChangeConnection;

	CVirtualMachine::MachineStateChangeEvent::Connection m_OnMachineStateChangeConnection;
	CVirtualMachine::RunningStateChangeEvent::Connection m_OnRunningStateChangeConnection;

	CDisAsmWnd::FindCallersRequestedEvent::Connection m_findCallersRequestConnection;

	Ui::DebuggerWindow* ui = nullptr;
	std::unique_ptr<CDebugView> m_debugView;
	CFunctionsView* m_functionsView = nullptr;
	CAddressListViewWnd* m_addressListView = nullptr;
	CPsfVm& m_virtualMachine;
};
