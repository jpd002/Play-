#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <boost/signal.hpp>
#include <string>
#include <memory>
#include "win32/Window.h"
#include "win32/StatusBar.h"
#include "SettingsDialogProvider.h"
#include "OutputWnd.h"
#include "AviStream.h"
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
	long							OnTimer(WPARAM);
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnActivateApp(bool, unsigned long);

private:
	class CScopedVmPauser
	{
	public:
						CScopedVmPauser(CPS2VM&);
		virtual			~CScopedVmPauser();

	private:
		CPS2VM&			m_virtualMachine;
		bool			m_paused;
	};

	class COpenCommand
	{
	public:
		virtual			~COpenCommand() {}
		virtual void	Execute(CMainWindow*) = 0;
	};

	class CBootCdRomOpenCommand : public COpenCommand
	{
	public:
		virtual void	Execute(CMainWindow*); 
	};

	class CLoadElfOpenCommand : public COpenCommand
	{
	public:
						CLoadElfOpenCommand(const char*);
		virtual void	Execute(CMainWindow*);

	private:
		std::string		m_fileName;
	};

	typedef std::shared_ptr<COpenCommand> OpenCommandPtr;

	void							OpenELF();
	void							BootCDROM();
	void							BootDiskImage();
	void							RecordAvi();
	void							ResumePause();
	void							Reset();
	void							PauseWhenFocusLost();
	void							SaveState();
	void							LoadState();
	void							ChangeFrameskip(bool);
	void							ChangeStateSlot(unsigned int);
	void							ShowDebugger();
	void							ShowSysInfo();
	void							ShowAbout();
	void							ShowSettingsDialog(CSettingsDialogProvider*);
	void							ShowRendererSettings();
	void							ShowControllerSettings();
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

	void							OnNewFrame(uint32);
	void							OnOutputWndSizeChange();
	void							OnExecutableChange();

	CPS2VM&							m_virtualMachine;

	unsigned int					m_nFrames;
	uint32							m_drawCallCount;
	HACCEL							m_nAccTable;

	unsigned int					m_nStateSlot;

	bool							m_nPauseFocusLost;
	bool							m_nDeactivatePause;

	OpenCommandPtr					m_lastOpenCommand;

	CAviStream						m_aviStream;
	bool							m_recordingAvi;
	uint8*							m_recordBuffer;
	HANDLE							m_recordAviMutex;
	unsigned int					m_recordBufferWidth;
	unsigned int					m_recordBufferHeight;

	Framework::Win32::CStatusBar*	m_pStatusBar;
	COutputWnd*						m_pOutputWnd;
#ifdef DEBUGGER_INCLUDED
	CDebugger*						m_pDebugger;
#endif
	static double					m_nStatusBarPanelWidths[2];
};

#endif
