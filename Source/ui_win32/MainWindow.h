#pragma once

#include <boost/signals2.hpp>
#include <string>
#include <memory>
#include "win32/Window.h"
#include "win32/StatusBar.h"
#include "SettingsDialogProvider.h"
#include "OutputWnd.h"
#include "AviStream.h"
#include "StatsOverlayWindow.h"
#ifdef DEBUGGER_INCLUDED
#include "Debugger.h"
#include "FrameDebugger/FrameDebugger.h"
#endif
#include "../Profiler.h"
#include "../PS2VM.h"

class CMainWindow : public Framework::Win32::CWindow, public boost::signals2::trackable
{
public:
									CMainWindow(CPS2VM&);
									~CMainWindow();
	int								Loop();

protected:
	long							OnTimer(WPARAM) override;
	long							OnCommand(unsigned short, unsigned short, HWND) override;
	long							OnActivateApp(bool, unsigned long) override;
	long							OnSize(unsigned int, unsigned int, unsigned int) override;
	long							OnMove(int, int) override;

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
	void							ChangeStateSlot(unsigned int);
	void							ChangeViewMode(CGSHandler::PRESENTATION_MODE);
	void							ShowDebugger();
	void							ShowFrameDebugger();
	void							DumpNextFrame();
	void							ToggleGsDraw();
	void							ShowSysInfo();
	void							ShowAbout();
	void							ShowSettingsDialog(CSettingsDialogProvider*);
	void							ShowRendererSettings();
	void							ShowControllerSettings();
	void							ShowVfsManager();
	void							ShowMcManager();
	void							ToggleSoundEnabled();

	void							ProcessCommandLine();

	void							LoadELF(const char*);
	void							RefreshLayout();
	void							RefreshStatsOverlayLayout();
	void							PrintVersion(TCHAR*, size_t);
	void							PrintStatusTextA(const char*, ...);
	void							SetStatusText(const TCHAR*);
	void							CreateAccelerators();
	
	void							CreateDebugMenu();
	static boost::filesystem::path	GetFrameDumpDirectoryPath();

	void							CreateStateSlotMenu();
	static boost::filesystem::path	GetStateDirectoryPath();
	boost::filesystem::path			GenerateStatePath() const;
	void							UpdateUI();

	void							OnNewFrame(uint32);

	void							OnOutputWndSizeChange();
	void							OnExecutableChange();

	void							SetupSoundHandler();

	CPS2VM&							m_virtualMachine;

	unsigned int					m_frames;
	uint32							m_drawCallCount;
	HACCEL							m_accTable;

	unsigned int					m_stateSlot;

	bool							m_pauseFocusLost;
	bool							m_deactivatePause;

	OpenCommandPtr					m_lastOpenCommand;

	CAviStream						m_aviStream;
	bool							m_recordingAvi;
	uint8*							m_recordBuffer;
	HANDLE							m_recordAviMutex;
	unsigned int					m_recordBufferWidth;
	unsigned int					m_recordBufferHeight;

	Framework::Win32::CStatusBar	m_statusBar;
	COutputWnd*						m_outputWnd;
	CStatsOverlayWindow				m_statsOverlayWnd;
#ifdef DEBUGGER_INCLUDED
	std::unique_ptr<CDebugger>		m_debugger;
	std::unique_ptr<CFrameDebugger>	m_frameDebugger;
#endif

	static double					m_statusBarPanelWidths[2];
};
