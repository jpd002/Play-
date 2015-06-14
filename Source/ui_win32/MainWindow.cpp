#include <stdio.h>
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <functional>
#include "string_format.h"
#include "string_cast.h"
#include "StdStreamUtils.h"
#include "PathUtils.h"
#include "win32/FileDialog.h"
#include "win32/AcceleratorTableGenerator.h"
#include "win32/MenuItem.h"
#include "win32/DpiUtils.h"
#include "MainWindow.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../AppConfig.h"
#include "../ee/PS2OS.h"
#include "../gs/GSH_Null.h"
#include "GSH_OpenGLWin32.h"
#include "PH_DirectInput.h"
#include "VFSManagerWnd.h"
#include "McManagerWnd.h"
#include "Debugger.h"
#include "SysInfoWnd.h"
#include "AboutWnd.h"
#include "resource.h"
#include "FileFilters.h"
#include "../../tools/PsfPlayer/Source/win32_ui/SH_WaveOut.h"

#define CLSNAME						_T("MainWindow")
#define WNDSTYLE					(WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX)

#define STATUSPANEL					0
#define FPSPANEL					1

#define FILEMENUPOS					0
#define VMMENUPOS					1
#define VIEWMENUPOS					2

#define ID_MAIN_VM_STATESLOT_0		(0xBEEF)
#define MAX_STATESLOTS				10

#define ID_MAIN_DEBUG_SHOWDEBUG			(0xDEAD)
#define ID_MAIN_DEBUG_SHOWFRAMEDEBUG	(0xDEAE)
#define ID_MAIN_DEBUG_DUMPFRAME			(0xDEAF)
#define ID_MAIN_DEBUG_ENABLEGSDRAW		(0xDEB0)

#define ID_MAIN_PROFILE_RESETSTATS		(0xDFAD)

#define PREF_UI_PAUSEWHENFOCUSLOST	"ui.pausewhenfocuslost"
#define PREF_UI_SOUNDENABLED		"ui.soundenabled"

double CMainWindow::m_statusBarPanelWidths[2] =
{
	0.7,
	0.3,
};

CMainWindow::CMainWindow(CPS2VM& virtualMachine)
: m_virtualMachine(virtualMachine)
, m_recordingAvi(false)
, m_recordBuffer(nullptr)
, m_recordBufferWidth(0)
, m_recordBufferHeight(0)
, m_recordAviMutex(NULL)
, m_frames(0)
, m_drawCallCount(0)
, m_stateSlot(0)
, m_outputWnd(nullptr)
, m_accTable(NULL)
{
	m_recordAviMutex = CreateMutex(NULL, FALSE, NULL);

	TCHAR sVersion[256];

	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST, true);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_UI_SOUNDENABLED, true);

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		wc.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&wc);
	}

	auto windowRect = Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 640, 480));
	Create(NULL, CLSNAME, _T(""), WNDSTYLE, windowRect, NULL, NULL);
	SetClassPtr();

#ifdef DEBUGGER_INCLUDED
	CDebugger::InitializeConsole();
#endif

	m_virtualMachine.Initialize();

	SetIcon(ICON_SMALL, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY)));
	SetIcon(ICON_BIG, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY)));

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINWINDOW)));

#ifdef DEBUGGER_INCLUDED
	m_debugger = std::unique_ptr<CDebugger>(new CDebugger(m_virtualMachine));
	m_frameDebugger = std::unique_ptr<CFrameDebugger>(new CFrameDebugger());
	CreateDebugMenu();
#endif

	PrintVersion(sVersion, countof(sVersion));

	m_outputWnd = new COutputWnd(m_hWnd);

	m_statsOverlayWnd = CStatsOverlayWindow(m_hWnd);

	m_statusBar = Framework::Win32::CStatusBar(m_hWnd);
	m_statusBar.SetParts(2, m_statusBarPanelWidths);
	m_statusBar.SetText(STATUSPANEL,	sVersion);
	m_statusBar.SetText(FPSPANEL,		_T(""));

	//m_virtualMachine.CreateGSHandler(CGSH_Null::GetFactoryFunction());
	m_virtualMachine.CreateGSHandler(CGSH_OpenGLWin32::GetFactoryFunction(m_outputWnd));

	m_virtualMachine.CreatePadHandler(CPH_DirectInput::GetFactoryFunction(m_hWnd));
	SetupSoundHandler();

	m_deactivatePause = false;
	m_pauseFocusLost = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST);

	m_virtualMachine.m_ee->m_gs->OnNewFrame.connect(boost::bind(&CMainWindow::OnNewFrame, this, _1));
	m_virtualMachine.ProfileFrameDone.connect(boost::bind(&CStatsOverlayWindow::OnProfileFrameDone, &m_statsOverlayWnd, _1));

	SetTimer(m_hWnd, NULL, 1000, NULL);
	//Initialize status bar
	OnTimer(0);

	m_virtualMachine.m_ee->m_os->OnExecutableChange.connect(boost::bind(&CMainWindow::OnExecutableChange, this));

	CreateStateSlotMenu();
	CreateAccelerators();

	ProcessCommandLine();

	RefreshLayout();

	UpdateUI();
	Center();
	Show(SW_SHOW);
#ifdef PROFILE
	m_statsOverlayWnd.Show(SW_SHOW);
#endif
}

CMainWindow::~CMainWindow()
{
	m_virtualMachine.Pause();

	m_virtualMachine.DestroyPadHandler();
	m_virtualMachine.DestroyGSHandler();
	m_virtualMachine.DestroySoundHandler();

#ifdef DEBUGGER_INCLUDED
	m_frameDebugger.reset();
	m_debugger.reset();
#endif

	delete m_outputWnd;

	DestroyAcceleratorTable(m_accTable);

	if(m_recordingAvi)
	{
		m_aviStream.Close();
		m_recordingAvi = false;
	}

	CloseHandle(m_recordAviMutex);
	m_recordAviMutex = NULL;

	m_virtualMachine.Destroy();
}

int CMainWindow::Loop()
{
	while(IsWindow())
	{
		MSG msg;
		GetMessage(&msg, NULL, 0, 0);
		bool nDispatched = false;
		HWND hActive = GetActiveWindow();

		if(hActive == m_hWnd)
		{
			nDispatched = TranslateAccelerator(m_hWnd, m_accTable, &msg) != 0;
		}
#ifdef DEBUGGER_INCLUDED
		else if(hActive == m_debugger->m_hWnd)
		{
			nDispatched = TranslateAccelerator(*m_debugger, m_debugger->GetAccelerators(), &msg) != 0;
		}
		else if(hActive == m_frameDebugger->m_hWnd)
		{
			nDispatched = TranslateAccelerator(*m_frameDebugger, m_frameDebugger->GetAccelerators(), &msg) != 0;
		}
#endif
		if(!nDispatched)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

long CMainWindow::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	switch(nID)
	{
	case ID_MAIN_FILE_LOADELF:
		OpenELF();
		break;
	case ID_MAIN_FILE_BOOTCDROM:
		BootCDROM();
		break;
	case ID_MAIN_FILE_BOOTDISKIMAGE:
		BootDiskImage();
		break;
	case ID_MAIN_FILE_RECORDAVI:
		RecordAvi();
		break;
	case ID_MAIN_FILE_EXIT:
		DestroyWindow(m_hWnd);
		break;
	case ID_MAIN_VM_RESUME:
		ResumePause();
		break;
	case ID_MAIN_VM_RESET:
		Reset();
		break;
	case ID_MAIN_VM_PAUSEFOCUS:
		PauseWhenFocusLost();
		break;
	case ID_MAIN_VM_SAVESTATE:
		SaveState();
		break;
	case ID_MAIN_VM_LOADSTATE:
		LoadState();
		break;
	case ID_MAIN_VM_STATESLOT_0 + 0:
	case ID_MAIN_VM_STATESLOT_0 + 1:
	case ID_MAIN_VM_STATESLOT_0 + 2:
	case ID_MAIN_VM_STATESLOT_0 + 3:
	case ID_MAIN_VM_STATESLOT_0 + 4:
	case ID_MAIN_VM_STATESLOT_0 + 5:
	case ID_MAIN_VM_STATESLOT_0 + 6:
	case ID_MAIN_VM_STATESLOT_0 + 7:
	case ID_MAIN_VM_STATESLOT_0 + 8:
	case ID_MAIN_VM_STATESLOT_0 + 9:
		ChangeStateSlot(nID - ID_MAIN_VM_STATESLOT_0);
		break;
	case ID_MAIN_VIEW_FITTOSCREEN:
		ChangeViewMode(CGSHandler::PRESENTATION_MODE_FIT);
		break;
	case ID_MAIN_VIEW_FILLSCREEN:
		ChangeViewMode(CGSHandler::PRESENTATION_MODE_FILL);
		break;
	case ID_MAIN_VIEW_ACTUALSIZE:
		ChangeViewMode(CGSHandler::PRESENTATION_MODE_ORIGINAL);
		break;
	case ID_MAIN_OPTIONS_RENDERER:
		ShowRendererSettings();
		break;
	case ID_MAIN_OPTIONS_CONTROLLER:
		ShowControllerSettings();
		break;
	case ID_MAIN_OPTIONS_VFSMANAGER:
		ShowVfsManager();
		break;
	case ID_MAIN_OPTIONS_MCMANAGER:
		ShowMcManager();
		break;
	case ID_MAIN_OPTIONS_ENABLESOUND:
		ToggleSoundEnabled();
		break;
	case ID_MAIN_DEBUG_SHOWDEBUG:
		ShowDebugger();
		break;
	case ID_MAIN_DEBUG_SHOWFRAMEDEBUG:
		ShowFrameDebugger();
		break;
	case ID_MAIN_DEBUG_DUMPFRAME:
		DumpNextFrame();
		break;
	case ID_MAIN_DEBUG_ENABLEGSDRAW:
		ToggleGsDraw();
		break;
#ifdef PROFILE
	case ID_MAIN_PROFILE_RESETSTATS:
		m_statsOverlayWnd.ResetStats();
		break;
#endif
	case ID_MAIN_HELP_SYSINFO:
		ShowSysInfo();
		break;
	case ID_MAIN_HELP_ABOUT:
		ShowAbout();
		break;

	}
	return TRUE;
}

long CMainWindow::OnTimer(WPARAM)
{
	uint32 dcpf = (m_frames != 0) ? (m_drawCallCount / m_frames) : 0;
	auto caption = string_format(_T("%d f/s, %d dc/f"), m_frames, dcpf);
	m_statusBar.SetText(FPSPANEL, caption.c_str());

#ifdef PROFILE
	m_statsOverlayWnd.Update(m_frames);
#endif

	m_frames = 0;
	m_drawCallCount = 0;

	return TRUE;
}

long CMainWindow::OnActivateApp(bool nActive, unsigned long nThreadId)
{
	if(m_pauseFocusLost == true)
	{
		if(nActive == false)
		{
			if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
			{
				ResumePause();
				m_deactivatePause = true;
			}
		}
		
		if((nActive == true) && (m_deactivatePause == true))
		{
			ResumePause();
			m_deactivatePause = false;
		}
	}
	return FALSE;
}

long CMainWindow::OnSize(unsigned int, unsigned int, unsigned int)
{
	if(m_outputWnd && m_statusBar)
	{
		RefreshLayout();
	}
	RefreshStatsOverlayLayout();
	return TRUE;
}

long CMainWindow::OnMove(int x, int y)
{
	RefreshStatsOverlayLayout();
	return FALSE;
}

void CMainWindow::OpenELF()
{
	Framework::Win32::CFileDialog d;
	d.m_OFN.lpstrFilter = _T("ELF Executable Files (*.elf)\0*.elf\0All files (*.*)\0*.*\0");

	Enable(FALSE);
	int nRet = d.SummonOpen(m_hWnd);
	Enable(TRUE);
	SetFocus();

	if(nRet == 0) return;

	LoadELF(string_cast<std::string>(d.GetPath()).c_str());
}

void CMainWindow::ResumePause()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		m_virtualMachine.Pause();
		SetStatusText(_T("Virtual Machine paused."));
	}
	else
	{
		m_virtualMachine.Resume();
		SetStatusText(_T("Virtual Machine resumed."));
	}
}

void CMainWindow::Reset()
{
	if(m_lastOpenCommand)
	{
		m_lastOpenCommand->Execute(this);
	}
}

void CMainWindow::PauseWhenFocusLost()
{
	m_pauseFocusLost = !m_pauseFocusLost;
	if(m_pauseFocusLost)
	{
		m_deactivatePause = false;
	}

	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_UI_PAUSEWHENFOCUSLOST, m_pauseFocusLost);
	UpdateUI();
}

void CMainWindow::SaveState()
{
	if(m_virtualMachine.m_ee->m_os->GetELF() == nullptr) return;

	Framework::PathUtils::EnsurePathExists(GetStateDirectoryPath());
	if(m_virtualMachine.SaveState(GenerateStatePath().string().c_str()) == 0)
	{
		PrintStatusTextA("Saved state to slot %i.", m_stateSlot);
	}
	else
	{
		PrintStatusTextA("Error saving state to slot %i.", m_stateSlot);
	}
}

void CMainWindow::LoadState()
{
	if(m_virtualMachine.m_ee->m_os->GetELF() == nullptr) return;

	if(m_virtualMachine.LoadState(GenerateStatePath().string().c_str()) == 0)
	{
		PrintStatusTextA("Loaded state from slot %i.", m_stateSlot);
	}
	else
	{
		PrintStatusTextA("Error loading state from slot %i.", m_stateSlot);
	}
}

void CMainWindow::ChangeStateSlot(unsigned int nSlot)
{
	m_stateSlot = nSlot % MAX_STATESLOTS;
	UpdateUI();
}

void CMainWindow::ChangeViewMode(CGSHandler::PRESENTATION_MODE presentationMode)
{
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, presentationMode);
	RefreshLayout();
	UpdateUI();
}

void CMainWindow::ShowDebugger()
{
#ifdef DEBUGGER_INCLUDED
	m_debugger->Show(SW_MAXIMIZE);
	SetForegroundWindow(m_debugger->m_hWnd);
#endif
}

void CMainWindow::ShowFrameDebugger()
{
#ifdef DEBUGGER_INCLUDED
	m_frameDebugger->Show(SW_MAXIMIZE);
	SetForegroundWindow(m_frameDebugger->m_hWnd);
#endif
}

void CMainWindow::DumpNextFrame()
{
	m_virtualMachine.TriggerFrameDump(
		[&] (const CFrameDump& frameDump)
		{
			try
			{
				auto frameDumpDirectoryPath = GetFrameDumpDirectoryPath();
				Framework::PathUtils::EnsurePathExists(frameDumpDirectoryPath);
				for(unsigned int i = 0; i < UINT_MAX; i++)
				{
					auto frameDumpFileName = string_format("framedump_%0.8d.dmp.zip", i);
					auto frameDumpPath = frameDumpDirectoryPath / boost::filesystem::path(frameDumpFileName);
					if(!boost::filesystem::exists(frameDumpPath))
					{
						auto dumpStream = Framework::CreateOutputStdStream(frameDumpPath.native());
						frameDump.Write(dumpStream);
						PrintStatusTextA("Dumped frame to '%s'.", frameDumpFileName.c_str());
						return;
					}
				}
			}
			catch(...)
			{

			}
			PrintStatusTextA("Failed to dump frame.");
		}
	);
}

void CMainWindow::ToggleGsDraw()
{
#ifdef DEBUGGER_INCLUDED
	auto gs = m_virtualMachine.GetGSHandler();
	if(gs == nullptr) return;
	bool newState = !gs->GetDrawEnabled();
	gs->SetDrawEnabled(newState);
	Framework::Win32::CMenuItem::FindById(GetMenu(m_hWnd), ID_MAIN_DEBUG_ENABLEGSDRAW).Check(newState);
	PrintStatusTextA(newState ? "GS Draw Enabled" : "GS Draw Disabled");
#endif
}

void CMainWindow::ShowSysInfo()
{
	{
		CSysInfoWnd SysInfoWnd(m_hWnd);
		SysInfoWnd.DoModal();
	}
	Redraw();
}

void CMainWindow::ShowAbout()
{
	{
		CAboutWnd AboutWnd(m_hWnd);
		AboutWnd.DoModal();
	}
	Redraw();
}

void CMainWindow::ShowSettingsDialog(CSettingsDialogProvider* provider)
{
	if(!provider) return;

	CScopedVmPauser vmPauser(m_virtualMachine);

	Framework::Win32::CModalWindow* pWindow = provider->CreateSettingsDialog(m_hWnd);
	pWindow->DoModal();
	delete pWindow;
	provider->OnSettingsDialogDestroyed();

	Redraw();
}

void CMainWindow::ShowRendererSettings()
{
	ShowSettingsDialog(dynamic_cast<CSettingsDialogProvider*>(m_virtualMachine.GetGSHandler()));
}

void CMainWindow::ShowControllerSettings()
{
	if(!m_virtualMachine.m_pad) return;
	ShowSettingsDialog(dynamic_cast<CSettingsDialogProvider*>(m_virtualMachine.m_pad));
}

void CMainWindow::ShowVfsManager()
{
	bool nPaused = false;

	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		nPaused = true;
		ResumePause();
	}

	CVFSManagerWnd VFSManagerWnd(m_hWnd);
	VFSManagerWnd.DoModal();

	Redraw();

	if(nPaused)
	{
		ResumePause();
	}
}

void CMainWindow::ShowMcManager()
{
	bool nPaused = false;

	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		nPaused = true;
		ResumePause();
	}

	CMcManagerWnd McManagerWnd(m_hWnd);
	McManagerWnd.DoModal();

	Redraw();

	if(nPaused)
	{
		ResumePause();
	}
}

void CMainWindow::ToggleSoundEnabled()
{
	auto soundEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_SOUNDENABLED);
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_UI_SOUNDENABLED, !soundEnabled);
	SetupSoundHandler();
	UpdateUI();
}

void CMainWindow::ProcessCommandLine()
{
	for(unsigned int i = 0; i < __argc; i++)
	{
		auto arg = __targv[i];
		if(_tcsstr(arg, _T("--cdrom0")) != nullptr)
		{
			BootCDROM();
		}
		else if(_tcsstr(arg, _T("--elf")) != nullptr)
		{
			if((__argc - i) < 2) continue;
			auto nextArg = __targv[i + 1];
			LoadELF(string_cast<std::string>(nextArg).c_str());
		}
#ifdef DEBUGGER_INCLUDED
		else if(_tcsstr(arg, _T("--debugger")) != nullptr)
		{
			ShowDebugger();
		}
		else if(_tcsstr(arg, _T("--framedebugger")) != nullptr)
		{
			ShowFrameDebugger();
		}
#endif
	}
}

void CMainWindow::LoadELF(const char* sFilename)
{
	CPS2OS& os = *m_virtualMachine.m_ee->m_os;
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();

	try
	{
		os.BootFromFile(sFilename);
#if !defined(_DEBUG) && !defined(DEBUGGER_INCLUDED)
		m_virtualMachine.Resume();
#endif
		m_lastOpenCommand = OpenCommandPtr(new CLoadElfOpenCommand(sFilename));
		PrintStatusTextA("Loaded executable '%s'.", os.GetExecutableName());
	}
	catch(const std::exception& Exception)
	{
		MessageBoxA(m_hWnd, Exception.what(), NULL, 16);
	}
}

void CMainWindow::BootCDROM()
{
	CPS2OS& os = *m_virtualMachine.m_ee->m_os;
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();

	try
	{
		os.BootFromCDROM(CPS2OS::ArgumentList());
#ifndef _DEBUG
		m_virtualMachine.Resume();
#endif
		m_lastOpenCommand = OpenCommandPtr(new CBootCdRomOpenCommand());
		PrintStatusTextA("Loaded executable '%s' from cdrom0.", os.GetExecutableName());
	}
	catch(const std::exception& Exception)
	{
		MessageBoxA(m_hWnd, Exception.what(), NULL, 16);
	}
}

void CMainWindow::BootDiskImage()
{
	Framework::Win32::CFileDialog d;
	d.m_OFN.lpstrFilter = DISKIMAGE_FILTER;

	Enable(FALSE);
	int nRet = d.SummonOpen(m_hWnd);
	Enable(TRUE);
	SetFocus();

	if(nRet == 0) return;

	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, string_cast<std::string>(d.GetPath()).c_str());
	BootCDROM();
}

void CMainWindow::RecordAvi()
{
	if(m_recordingAvi)
	{
		m_recordingAvi = false;

		while(1)
		{
			DWORD result = MsgWaitForMultipleObjects(1, &m_recordAviMutex, FALSE, INFINITE, QS_ALLINPUT);
			if(result == WAIT_OBJECT_0) break;
			MSG msg;
			GetMessage(&msg, NULL, 0, 0);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		{
			m_aviStream.Close();
			delete [] m_recordBuffer;
			m_recordBuffer = NULL;
			m_recordBufferWidth = 0;
			m_recordBufferHeight = 0;
		}
		ReleaseMutex(m_recordAviMutex);

		UpdateUI();
	}
	else
	{
		CScopedVmPauser vmPauser(m_virtualMachine);

		Framework::Win32::CFileDialog d;
		d.m_OFN.lpstrFilter = _T("AVI Movie Files (*.avi)\0*.avi\0All files (*.*)\0*.*\0");
		if(!d.SummonSave(m_hWnd))
		{
			return;
		}

		RECT rc = m_outputWnd->GetWindowRect();
		unsigned int nViewW = rc.right - rc.left;
		unsigned int nViewH = rc.bottom - rc.top;

		m_aviStream.SetSize(nViewW, nViewH);
		if(!m_aviStream.Open(m_hWnd, d.GetPath()))
		{
			MessageBox(m_hWnd, _T("Failed to start AVI recording."), APP_NAME, 16);
			return;
		}

		m_recordBufferWidth = nViewW;
		m_recordBufferHeight = nViewH;
		m_recordBuffer = new uint8[m_recordBufferWidth * m_recordBufferHeight * 4];

		m_recordingAvi = true;
		UpdateUI();
	}
}

void CMainWindow::RefreshLayout()
{
	auto clientRect = GetClientRect();

	unsigned int outputWidth = clientRect.Width();
	unsigned int outputHeight = std::max<int>(clientRect.Height() - m_statusBar.GetHeight(), 0);

	m_outputWnd->SetSize(outputWidth, outputHeight);

	m_statusBar.RefreshGeometry();
	m_statusBar.SetParts(2, m_statusBarPanelWidths);

	{
		auto presentationMode = static_cast<CGSHandler::PRESENTATION_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));

		CGSHandler::PRESENTATION_PARAMS presentationParams;
		presentationParams.mode = presentationMode;
		presentationParams.windowWidth = outputWidth;
		presentationParams.windowHeight = outputHeight;
		m_virtualMachine.m_ee->m_gs->SetPresentationParams(presentationParams);
		m_virtualMachine.m_ee->m_gs->Flip(true);
	}
}

void CMainWindow::RefreshStatsOverlayLayout()
{
	auto clientRect = GetClientRect();

	unsigned int outputWidth = clientRect.Width();
	unsigned int outputHeight = std::max<int>(clientRect.Height() - m_statusBar.GetHeight(), 0);

	auto clientScreenRect = Framework::Win32::CRect(0, 0, outputWidth, outputHeight);
	clientScreenRect.ClientToScreen(m_hWnd);
	SetWindowPos(m_statsOverlayWnd.m_hWnd, NULL, 
		clientScreenRect.Left(), clientScreenRect.Top(), 
		clientScreenRect.Width(), clientScreenRect.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE);
}

void CMainWindow::PrintStatusTextA(const char* format, ...)
{
	char text[256];
	va_list args;

	va_start(args, format);
	_vsnprintf(text, 256, format, args);
	va_end(args);

	m_statusBar.SetText(STATUSPANEL, string_cast<std::tstring>(text).c_str());
}

void CMainWindow::SetStatusText(const TCHAR* text)
{
	m_statusBar.SetText(STATUSPANEL, text);
}

void CMainWindow::CreateAccelerators()
{
	Framework::Win32::CAcceleratorTableGenerator generator;
	generator.Insert(ID_MAIN_FILE_LOADELF,				'O',			FVIRTKEY | FCONTROL);
	generator.Insert(ID_MAIN_VM_RESUME,					VK_F5,			FVIRTKEY);
	generator.Insert(ID_MAIN_VM_SAVESTATE,				VK_F7,			FVIRTKEY);
	generator.Insert(ID_MAIN_VM_LOADSTATE,				VK_F8,			FVIRTKEY);
	generator.Insert(ID_MAIN_VIEW_FITTOSCREEN,			'J',			FVIRTKEY | FCONTROL);
	generator.Insert(ID_MAIN_VIEW_FILLSCREEN,			'K',			FVIRTKEY | FCONTROL);
	generator.Insert(ID_MAIN_VIEW_ACTUALSIZE,			'L',			FVIRTKEY | FCONTROL);
	generator.Insert(ID_MAIN_DEBUG_DUMPFRAME,			VK_F11,			FVIRTKEY);
#ifdef PROFILE
	generator.Insert(ID_MAIN_PROFILE_RESETSTATS,		VK_F3,			FVIRTKEY);
#endif
	m_accTable = generator.Create();
}

void CMainWindow::CreateDebugMenu()
{
	HMENU hMenu = CreatePopupMenu();
	InsertMenu(hMenu, 0, MF_STRING,					ID_MAIN_DEBUG_SHOWDEBUG,		_T("Show Debugger"));
	InsertMenu(hMenu, 1, MF_SEPARATOR,				0,								nullptr);
	InsertMenu(hMenu, 2, MF_STRING,					ID_MAIN_DEBUG_DUMPFRAME,		_T("Dump Next Frame\tF11"));
	InsertMenu(hMenu, 3, MF_STRING,					ID_MAIN_DEBUG_SHOWFRAMEDEBUG,	_T("Show Frame Debugger"));
	InsertMenu(hMenu, 4, MF_STRING | MF_CHECKED,	ID_MAIN_DEBUG_ENABLEGSDRAW,		_T("GS Draw Enabled"));

	MENUITEMINFO ItemInfo;
	memset(&ItemInfo, 0, sizeof(MENUITEMINFO));
	ItemInfo.cbSize		= sizeof(MENUITEMINFO);
	ItemInfo.fMask		= MIIM_STRING | MIIM_SUBMENU;
	ItemInfo.dwTypeData	= _T("Debug");
	ItemInfo.hSubMenu	= hMenu;

	InsertMenuItem(GetMenu(m_hWnd), 3, TRUE, &ItemInfo);
}

boost::filesystem::path CMainWindow::GetFrameDumpDirectoryPath()
{
	return CAppConfig::GetBasePath() / boost::filesystem::path("framedumps/");
}

void CMainWindow::CreateStateSlotMenu()
{
	HMENU hMenu = CreatePopupMenu();
	for(unsigned int i = 0; i < MAX_STATESLOTS; i++)
	{
		std::tstring sCaption = _T("Slot ") + boost::lexical_cast<std::tstring>(i);
		InsertMenu(hMenu, i, MF_STRING, ID_MAIN_VM_STATESLOT_0 + i, sCaption.c_str());
	}

	MENUITEMINFO ItemInfo;
	memset(&ItemInfo, 0, sizeof(MENUITEMINFO));
	ItemInfo.cbSize		= sizeof(MENUITEMINFO);
	ItemInfo.fMask		= MIIM_SUBMENU;
	ItemInfo.hSubMenu	= hMenu;

	hMenu = GetSubMenu(GetMenu(m_hWnd), VMMENUPOS);
	SetMenuItemInfo(hMenu, ID_MAIN_VM_STATESLOT, FALSE, &ItemInfo);
}

boost::filesystem::path CMainWindow::GetStateDirectoryPath()
{
	return CAppConfig::GetBasePath() / boost::filesystem::path("states/");
}

boost::filesystem::path CMainWindow::GenerateStatePath() const
{
	std::string stateFileName = std::string(m_virtualMachine.m_ee->m_os->GetExecutableName()) + ".st" + std::to_string(m_stateSlot) + ".zip";
	return GetStateDirectoryPath() / boost::filesystem::path(stateFileName);
}

void CMainWindow::UpdateUI()
{
	CPS2OS& os = *m_virtualMachine.m_ee->m_os;
	Framework::Win32::CMenuItem mainMenu(GetMenu(m_hWnd));

	//Fix the file menu
	{
		HMENU hMenu = GetSubMenu(GetMenu(m_hWnd), FILEMENUPOS);

		ModifyMenu(hMenu, ID_MAIN_FILE_RECORDAVI, MF_BYCOMMAND | MF_STRING, ID_MAIN_FILE_RECORDAVI, m_recordingAvi ? _T("Stop Recording AVI") : _T("Record AVI...")); 
	}

	//Fix the virtual machine sub menu
	{
		HMENU hMenu = GetSubMenu(GetMenu(m_hWnd), VMMENUPOS);

		bool hasElf = (os.GetELF() != NULL);

		EnableMenuItem(hMenu, ID_MAIN_VM_RESUME, (!hasElf ? MF_GRAYED : 0) | MF_BYCOMMAND);
		EnableMenuItem(hMenu, ID_MAIN_VM_RESET, (!hasElf ? MF_GRAYED : 0) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, ID_MAIN_VM_PAUSEFOCUS, (m_pauseFocusLost ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		EnableMenuItem(hMenu, ID_MAIN_VM_SAVESTATE, (!hasElf ? MF_GRAYED : 0) | MF_BYCOMMAND);
		EnableMenuItem(hMenu, ID_MAIN_VM_LOADSTATE, (!hasElf ? MF_GRAYED : 0) | MF_BYCOMMAND);

		//Get state slot sub-menu
		MENUITEMINFO MenuItem;
		memset(&MenuItem, 0, sizeof(MENUITEMINFO));
		MenuItem.cbSize = sizeof(MENUITEMINFO);
		MenuItem.fMask	= MIIM_SUBMENU;

		GetMenuItemInfo(hMenu, ID_MAIN_VM_STATESLOT, FALSE, &MenuItem);
		hMenu = MenuItem.hSubMenu;

		//Change state slot number checkbox
		for(unsigned int i = 0; i < MAX_STATESLOTS; i++)
		{
			memset(&MenuItem, 0, sizeof(MENUITEMINFO));
			MenuItem.cbSize = sizeof(MENUITEMINFO);
			MenuItem.fMask	= MIIM_STATE;
			MenuItem.fState	= (m_stateSlot == i) ? MFS_CHECKED : MFS_UNCHECKED;

			SetMenuItemInfo(hMenu, ID_MAIN_VM_STATESLOT_0 + i, FALSE, &MenuItem);
		}
	}

	//Fix the view menu
	{
		auto presentationMode = static_cast<CGSHandler::PRESENTATION_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));

		HMENU viewMenu = GetSubMenu(GetMenu(m_hWnd), VIEWMENUPOS);
		CheckMenuItem(viewMenu, ID_MAIN_VIEW_FITTOSCREEN, ((presentationMode == CGSHandler::PRESENTATION_MODE_FIT) ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(viewMenu, ID_MAIN_VIEW_FILLSCREEN, ((presentationMode == CGSHandler::PRESENTATION_MODE_FILL) ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(viewMenu, ID_MAIN_VIEW_ACTUALSIZE, ((presentationMode == CGSHandler::PRESENTATION_MODE_ORIGINAL) ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	}

	//Fix the options menu
	{
		auto enableSoundMenuItem = mainMenu.FindById(ID_MAIN_OPTIONS_ENABLESOUND);
		enableSoundMenuItem.Check(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_SOUNDENABLED));
	}

	TCHAR sTitle[256];
	const char* sExec = os.GetExecutableName();
	if(strlen(sExec))
	{
		_sntprintf(sTitle, countof(sTitle), _T("%s - [ %s ]"), APP_NAME, string_cast<std::tstring>(sExec).c_str());
	}
	else
	{
		_sntprintf(sTitle, countof(sTitle), _T("%s"), APP_NAME);
	}

	SetText(sTitle);
}

void CMainWindow::PrintVersion(TCHAR* sVersion, size_t nCount)
{
	_sntprintf(sVersion, nCount, APP_NAME _T(" v%s - %s"), APP_VERSIONSTR, string_cast<std::tstring>(__DATE__).c_str());
}

void CMainWindow::OnNewFrame(uint32 drawCallCount)
{
	if(m_recordingAvi)
	{
		WaitForSingleObject(m_recordAviMutex, INFINITE);

		m_virtualMachine.m_ee->m_gs->ReadFramebuffer(m_recordBufferWidth, m_recordBufferHeight, m_recordBuffer);
		m_aviStream.Write(m_recordBuffer);

		ReleaseMutex(m_recordAviMutex);
	}

	m_frames++;
	m_drawCallCount += drawCallCount;
}

void CMainWindow::OnExecutableChange()
{
	UpdateUI();
}

void CMainWindow::SetupSoundHandler()
{
	auto soundEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_UI_SOUNDENABLED);
	if(soundEnabled)
	{
		m_virtualMachine.CreateSoundHandler(&CSH_WaveOut::HandlerFactory);
	}
	else
	{
		m_virtualMachine.DestroySoundHandler();
	}
}

CMainWindow::CScopedVmPauser::CScopedVmPauser(CPS2VM& virtualMachine)
: m_virtualMachine(virtualMachine)
, m_paused(false)
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		m_paused = true;
		m_virtualMachine.Pause();
	}
}

CMainWindow::CScopedVmPauser::~CScopedVmPauser()
{
	if(m_paused)
	{
		m_virtualMachine.Resume();
	}
}

void CMainWindow::CBootCdRomOpenCommand::Execute(CMainWindow* mainWindow)
{
	mainWindow->BootCDROM();
}

CMainWindow::CLoadElfOpenCommand::CLoadElfOpenCommand(const char* fileName) :
m_fileName(fileName)
{

}

void CMainWindow::CLoadElfOpenCommand::Execute(CMainWindow* mainWindow)
{
	mainWindow->LoadELF(m_fileName.c_str());
}
