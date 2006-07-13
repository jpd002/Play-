#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include "win32/MDIFrame.h"
#include "ELFView.h"
#include "FunctionsView.h"
#include "DebugView.h"
#include "Log.h"
#include "EventHandler.h"

class CDebugger : public Framework::CMDIFrame
{
public:
									CDebugger();
									~CDebugger();
	HACCEL							GetAccelerators();

protected:
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnSysCommand(unsigned int, LPARAM);
	long							OnWndProc(unsigned int, WPARAM, LPARAM);

private:
	enum DEBUGVIEW
	{
		DEBUGVIEW_EE	= 0,
		DEBUGVIEW_VU0,
		DEBUGVIEW_VU1,
		DEBUGVIEW_MAX,
	};

	void							RegisterPreferences();
	void							UpdateLoggingMenu();
	void							UpdateTitle();
	void							LoadSettings();
	void							SaveSettings();
	void							SerializeWindowGeometry(Framework::CWindow*, const char*, const char*, const char*, const char*, const char*);
	void							UnserializeWindowGeometry(Framework::CWindow*, const char*, const char*, const char*, const char*, const char*);
	void							CreateAccelerators();
	void							DestroyAccelerators();
	void							StartShiftOpTest();
	void							StartShift64OpTest();
	void							StartSplitLoadOpTest();
	void							StartAddition64OpTest();
	void							StartSetLessThanOpTest();
	void							Resume();
	void							StepCPU1();
	void							FindValue();
	void							AssembleJAL();
	void							Layout1024();
	void							Layout1280();

	//View related functions
	void							ActivateView(unsigned int);
	void							LoadViewLayout();
	void							SaveViewLayout();

	CDebugView*						GetCurrentView();
	CMIPS*							GetContext();
	CDisAsmWnd*						GetDisassemblyWindow();
	CMemoryViewMIPSWnd*				GetMemoryViewWindow();
	CRegViewWnd*					GetRegisterViewWindow();
	CCallStackWnd*					GetCallStackWindow();

	//Event handlers
	void							OnFunctionViewFunctionDblClick(uint32);
	void							OnFunctionViewFunctionsStateChange(int);
	void							OnExecutableChange(int);
	void							OnExecutableUnloading(int);

	//Tunnelled handlers
	void							OnExecutableChangeMsg();
	void							OnExecutableUnloadingMsg();

	Framework::CEventHandler<int>*	m_pOnExecutableChangeHandler;
	Framework::CEventHandler<int>*	m_pOnExecutableUnloadingHandler;

	HACCEL							m_nAccTable;

	CELFView*						m_pELFView;
	CFunctionsView*					m_pFunctionsView;
	CLog*							m_pLog;
	CDebugView*						m_pView[DEBUGVIEW_MAX];
	unsigned int					m_nCurrentView;
};

#endif