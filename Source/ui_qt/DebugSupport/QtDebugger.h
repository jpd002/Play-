#pragma once

#include <QMainWindow>
#include <QCloseEvent>

#include "DebugView.h"
#include "CallStackWnd.h"
#include "FunctionsView.h"
#include "ThreadsViewWnd.h"
#include "AddressListViewWnd.h"
#include "PS2VM.h"
#include "ELFView.h"

// Predeclares to avoid headers
class CDebugView;

namespace Ui
{
	class QtDebugger;
}

class QtDebugger : public QMainWindow
{
	Q_OBJECT

public:
	explicit QtDebugger(CPS2VM&);
	~QtDebugger();

protected:
	void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;

private slots:
	void on_actionResume_triggered();
	void on_actionStep_CPU_triggered();
	void on_actionDump_INTC_Handlers_triggered();
	void on_actionDump_DMAC_Handlers_triggered();
	void on_actionAssemble_JAL_triggered();
	void on_actionReanalyse_ee_triggered();
	void on_actionFind_Functions_triggered();

	void on_actionCascade_triggered();
	void on_actionTile_triggered();
	void on_actionLayout_1024x768_triggered();
	void on_actionLayout_1280x800_triggered();
	void on_actionLayout_1280x1024_triggered();
	void on_actionLayout_1600x1200_triggered();

	void on_actionfind_word_value_triggered();
	void on_actionFind_Word_Half_Value_triggered();

	void on_actionCall_Stack_triggered();
	void on_actionFunctions_triggered();
	void on_actionELF_File_Information_triggered();
	void on_actionThreads_triggered();
	void on_actionView_Disassmebly_triggered();
	void on_actionView_Registers_triggered();
	void on_actionMemory_triggered();

	void on_actionEmotionEngine_View_triggered();
	void on_actionVector_Unit_0_triggered();
	void on_actionVector_Unit_1_triggered();
	void on_actionIOP_View_triggered();

signals:
	void OnExecutableChange();
	void OnExecutableUnloading();
	void OnMachineStateChange();
	void OnRunningStateChange();

private:
	Ui::QtDebugger* ui;

	enum DEBUGVIEW
	{
		DEBUGVIEW_EE = 0,
		DEBUGVIEW_VU0,
		DEBUGVIEW_VU1,
		DEBUGVIEW_IOP,
		DEBUGVIEW_MAX,
	};

	void RegisterPreferences();
	void UpdateTitle();
	void LoadSettings();
	void SaveSettings();
	void SerializeWindowGeometry(QWidget*, const char*, const char*, const char*, const char*, const char*);
	void UnserializeWindowGeometry(QWidget*, const char*, const char*, const char*, const char*, const char*);
	void Resume();
	void StepCPU();
	void FindWordValue(uint32);
	void AssembleJAL();
	void ReanalyzeEe();
	void FindEeFunctions();
	void Layout1024x768();
	void Layout1280x800();
	void Layout1280x1024();
	void Layout1600x1200();
	void LoadDebugTags();
	void SaveDebugTags();

	//View related functions
	void ActivateView(unsigned int);
	void LoadViewLayout();
	void SaveViewLayout();

	void LoadBytesPerLine();
	void SaveBytesPerLine();

	CDebugView* GetCurrentView();
	CDisAsmWnd* GetDisassemblyWindow();
	CMemoryViewMIPSWnd* GetMemoryViewWindow();
	CRegViewWnd* GetRegisterViewWindow();
	CCallStackWnd* GetCallStackWindow();

	//Search functions
	static std::vector<uint32> FindCallers(CMIPS*, uint32);
	static std::vector<uint32> FindWordValueRefs(CMIPS*, uint32, uint32);

	//Event handlers
	void OnFunctionsViewFunctionDblClick(uint32);
	void OnFunctionsViewFunctionsStateChange();
	void OnThreadsViewAddressDblClick(uint32);
	void OnExecutableChangeMsg();
	void OnExecutableUnloadingMsg();
	void OnMachineStateChangeMsg();
	void OnRunningStateChangeMsg();
	void OnFindCallersRequested(uint32);
	void OnFindCallersAddressDblClick(uint32);

	Framework::CSignal<void(uint32)>::Connection m_OnFunctionDblClickConnection;
	Framework::CSignal<void()>::Connection m_OnFunctionsStateChangeConnection;
	Framework::CSignal<void(uint32)>::Connection m_OnGotoAddressConnection;

	CAddressListViewWnd::AddressSelectedEvent::Connection m_AddressSelectedConnection;
	Framework::CSignal<void()>::Connection m_OnExecutableChangeConnection;
	Framework::CSignal<void()>::Connection m_OnExecutableUnloadingConnection;
	CVirtualMachine::MachineStateChangeEvent::Connection m_OnMachineStateChangeConnection;
	CVirtualMachine::RunningStateChangeEvent::Connection m_OnRunningStateChangeConnection;

	CDisAsmWnd::FindCallersRequestedEvent::Connection m_findCallersRequestConnection;

	CELFView* m_pELFView = nullptr;
	CFunctionsView* m_pFunctionsView = nullptr;
	QMdiSubWindow* m_threadsView = nullptr;
	CDebugView* m_pView[DEBUGVIEW_MAX];
	CAddressListViewWnd* m_addressListView = nullptr;
	unsigned int m_nCurrentView;
	CPS2VM& m_virtualMachine;
};
