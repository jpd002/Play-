#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include "win32/MDIFrame.h"
#include "ELFView.h"
#include "FunctionsView.h"
#include "ThreadsViewWnd.h"
#include "DebugView.h"
#include "../PS2VM.h"

class CDebugger : public Framework::Win32::CMDIFrame, public boost::signals2::trackable
{
public:
									CDebugger(CPS2VM&);
	virtual							~CDebugger();
	HACCEL							GetAccelerators();
	static void						InitializeConsole();

protected:
	long							OnCommand(unsigned short, unsigned short, HWND) override;
	long							OnSysCommand(unsigned int, LPARAM) override;
	long							OnWndProc(unsigned int, WPARAM, LPARAM) override;

private:
	enum DEBUGVIEW
	{
		DEBUGVIEW_EE	= 0,
		DEBUGVIEW_VU0,
		DEBUGVIEW_VU1,
		DEBUGVIEW_IOP,
		DEBUGVIEW_MAX,
	};

	void							RegisterPreferences();
	void							UpdateLoggingMenu();
	void							UpdateTitle();
	void							LoadSettings();
	void							SaveSettings();
	void							SerializeWindowGeometry(Framework::Win32::CWindow*, const char*, const char*, const char*, const char*, const char*);
	void							UnserializeWindowGeometry(Framework::Win32::CWindow*, const char*, const char*, const char*, const char*, const char*);
	void							CreateAccelerators();
	void							DestroyAccelerators();
	void							Resume();
	void							StepCPU();
	void							FindValue();
	void							AssembleJAL();
	void							ReanalyzeEe();
	void							FindEeFunctions();
	void							Layout1024();
	void							Layout1280();
	void							Layout1600();
	void							LoadDebugTags();
	void							SaveDebugTags();

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
	void							OnFunctionsViewFunctionDblClick(uint32);
	void							OnFunctionsViewFunctionsStateChange();
	void							OnThreadsViewAddressDblClick(uint32);
	void							OnExecutableChange();
	void							OnExecutableUnloading();

	//Tunnelled handlers
	void							OnExecutableChangeMsg();
	void							OnExecutableUnloadingMsg();

	HACCEL							m_nAccTable;

	CELFView*						m_pELFView;
	CFunctionsView*					m_pFunctionsView;
	CThreadsViewWnd*				m_threadsView;
	CDebugView*						m_pView[DEBUGVIEW_MAX];
	unsigned int					m_nCurrentView;
	CPS2VM&							m_virtualMachine;
};

#endif
