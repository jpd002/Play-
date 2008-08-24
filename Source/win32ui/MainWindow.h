#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <boost/signal.hpp>
#include <string>
#include "win32/Window.h"
#include "win32/StatusBar.h"
#include "SettingsDialogProvider.h"
#include "OutputWnd.h"
#ifdef DEBUGGER_INCLUDED
#include "Debugger.h"
#endif
#include "../PS2VM.h"

class CMainWindow : public Framework::Win32::CWindow, public boost::signals::trackable
{
public:
									CMainWindow(CPS2VM&, char*);
									~CMainWindow();
	int								Loop();

protected:
	long							OnTimer();
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnActivateApp(bool, unsigned long);

private:
	void							OpenELF();
	void							BootCDROM();
	void							ResumePause();
	void							Reset();
	void							PauseWhenFocusLost();
	void							SaveState();
	void							LoadState();
    void                            ChangeFrameskip(bool);
	void							ChangeStateSlot(unsigned int);
	void							ShowDebugger();
	void							ShowSysInfo();
	void							ShowAbout();
    void                            ShowSettingsDialog(CSettingsDialogProvider*);
	void							ShowRendererSettings();
    void                            ShowControllerSettings();
	void							ShowVfsManager();
	void							ShowMcManager();

	void							LoadELF(const char*);
	void							RefreshLayout();
	void							PrintVersion(TCHAR*, size_t);
	void							PrintStatusTextA(const char*, ...);
	void							SetStatusText(const TCHAR*);
	void							CreateAccelerators();
	
	void							CreateDebugMenu();

	void							CreateStateSlotMenu();
	std::string						GenerateStatePath();
	void							UpdateUI();

	void							OnNewFrame();
	void							OnOutputWndSizeChange();
	void							OnExecutableChange();

    CPS2VM&                         m_virtualMachine;

    unsigned int					m_nFrames;
	HACCEL							m_nAccTable;

	unsigned int					m_nStateSlot;

	bool							m_nPauseFocusLost;
	bool							m_nDeactivatePause;

	Framework::Win32::CStatusBar*	m_pStatusBar;
	COutputWnd*						m_pOutputWnd;
#ifdef DEBUGGER_INCLUDED
	CDebugger*						m_pDebugger;
#endif
	static double					m_nStatusBarPanelWidths[2];
};

#endif
