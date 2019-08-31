#pragma once

#include <QDockWidget>
#include <QMdiArea>
#include <QGridLayout>
//#include "ELFView.h"
//#include "FunctionsView.h"
//#include "ThreadsViewWnd.h"
//#include "./Debugger/AddressListViewWnd.h"
#include "PS2VM.h"

// Predeclares to avoid headers
class CDebugView;
//class QDockWidget;
//class QGridLayout;

class CDebugger : public QDockWidget
{
public:
	CDebugger(QWidget*, CPS2VM&);
	virtual ~CDebugger();
	//HACCEL GetAccelerators();
	static void InitializeConsole();

protected:
	//long OnCommand(unsigned short, unsigned short, HWND) override;
	//long OnSysCommand(unsigned int, LPARAM) override;
	//LRESULT OnWndProc(unsigned int, WPARAM, LPARAM) override;

private:
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
	void SerializeWindowGeometry(const char*, const char*, const char*, const char*, const char*);
	void UnserializeWindowGeometry(const char*, const char*, const char*, const char*, const char*);
	void CreateAccelerators();
	void DestroyAccelerators();
	void Resume();
	void StepCPU();
	void FindWordValue(uint32);
	void AssembleJAL();
	void ReanalyzeEe();
	void FindEeFunctions();
	void Layout1024();
	void Layout1280();
	void Layout1600();
	void LoadDebugTags();
	void SaveDebugTags();

	//View related functions
	void ActivateView(unsigned int);
	void LoadViewLayout();
	void SaveViewLayout();

	void LoadBytesPerLine();
	void SaveBytesPerLine();

	CDebugView* GetCurrentView();
	CMIPS* GetContext();
	//CDisAsmWnd* GetDisassemblyWindow();
	//CMemoryViewMIPSWnd* GetMemoryViewWindow();
	//CRegViewWnd* GetRegisterViewWindow();
	//CCallStackWnd* GetCallStackWindow();

	//Search functions
	static std::vector<uint32> FindCallers(CMIPS*, uint32);
	static std::vector<uint32> FindWordValueRefs(CMIPS*, uint32, uint32);

	//Event handlers
	void OnFunctionsViewFunctionDblClick(uint32);
	void OnFunctionsViewFunctionsStateChange();
	void OnThreadsViewAddressDblClick(uint32);
	void OnExecutableChange();
	void OnExecutableUnloading();
	void OnMachineStateChange();
	void OnRunningStateChange();
	void OnFindCallersRequested(uint32);
	void OnFindCallersAddressDblClick(uint32);

	//Tunnelled handlers
	void OnExecutableChangeMsg();
	void OnExecutableUnloadingMsg();
	void OnMachineStateChangeMsg();
	void OnRunningStateChangeMsg();

	//HACCEL m_nAccTable;

	Framework::CSignal<void(uint32)>::Connection m_OnFunctionDblClickConnection;
	Framework::CSignal<void()>::Connection m_OnFunctionsStateChangeConnection;
	Framework::CSignal<void(uint32)>::Connection m_OnGotoAddressConnection;

	//CAddressListViewWnd::AddressSelectedEvent::Connection m_AddressSelectedConnection;
	Framework::CSignal<void()>::Connection m_OnExecutableChangeConnection;
	Framework::CSignal<void()>::Connection m_OnExecutableUnloadingConnection;
	Framework::CSignal<void()>::Connection m_OnMachineStateChangeConnection;
	Framework::CSignal<void()>::Connection m_OnRunningStateChangeConnection;

	//CDisAsm::FindCallersRequestedEvent::Connection m_findCallersRequestConnection;

	//CELFView* m_pELFView = nullptr;
	//CFunctionsView* m_pFunctionsView = nullptr;
	//CThreadsViewWnd* m_threadsView = nullptr;
	CDebugView* m_pView[DEBUGVIEW_MAX];
	//CAddressListViewWnd* m_addressListView = nullptr;
	unsigned int m_nCurrentView;
	CPS2VM& m_virtualMachine;

	// QT Stuff
	QMdiArea* m_debuggerMdi;
};
