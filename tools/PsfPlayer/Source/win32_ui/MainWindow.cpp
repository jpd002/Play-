#include "AppDef.h"
#include "AppConfig.h"
#include "MainWindow.h"
#include "AboutWindow.h"
#include "../PsfLoader.h"
#include "../PsfArchive.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "win32/MenuItem.h"
#include "win32/AcceleratorTableGenerator.h"
#include "layout/LayoutEngine.h"
#include "placeholder_def.h"
#include "MemStream.h"
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "resource.h"
#include "stricmp.h"

//#define FORCE_ENABLE_TRAYICON

#define ID_FILE_AUDIOPLUGIN_PLUGIN_0	(0xBEEF)
#define ID_FILE_CHARENCODING_ENCODING_0	(0xCEEF)

#define PLAYLIST_EXTENSION				_T(".psfpl")
#define PLAYLIST_FILTER					_T("PsfPlayer Playlists (*") PLAYLIST_EXTENSION _T(")\0*") PLAYLIST_EXTENSION _T("\0")
#define PSF_FILTER						_T("PlayStation Sound Files (*.psf; *.minipsf)\0*.psf; *.minipsf\0")
#define PSF2_FILTER						_T("PlayStation2 Sound Files (*.psf2; *.minipsf2)\0*.psf2; *.minipsf2\0")
#define PSFP_FILTER						_T("PlayStation Portable Sound Files (*.psfp; *.minipsfp)\0*.psfp; *.minipsfp\0")
#define ARCHIVE_FILTER					_T("Archived Sound Files (*.zip; *.rar)\0*.zip;*.rar\0")

#define TEXT_PLAY						_T("Play")
#define TEXT_PAUSE						_T("Pause")

#define TEXT_SYMBOL_PLAY				_T("4")
#define TEXT_SYMBOL_PAUSE				_T(";")

#define PREF_REVERB_ENABLED				("reverb.enabled")

#define PREF_SOUNDHANDLER_ID			("soundhandler.id")
#define DEFAULT_SOUND_HANDLER_ID		(1)

#define PREF_CHAR_ENCODING_ID			("charencoding.id")
#define DEFAULT_CHAR_ENCODING_ID		(CPsfTags::CE_WINDOWS_1252)

#define PREF_REPEAT_MODE				("repeat.mode")
#define DEFAULT_REPEAT_MODE				(CMainWindow::PLAYLIST_ONCE)

CMainWindow::SOUNDHANDLER_INFO CMainWindow::m_handlerInfo[] =
{
	{	1,		_T("Win32 WaveOut"),	_T("SH_WaveOut.dll")	},
	{	2,		_T("OpenAL"),			_T("SH_OpenAL.dll")		},
	{	NULL,	NULL,					NULL					},
};

CMainWindow::CHARENCODING_INFO CMainWindow::m_charEncodingInfo[] =
{
	{	CPsfTags::CE_WINDOWS_1252,	_T("Windows 1252")		},
	{	CPsfTags::CE_SHIFT_JIS,		_T("Shift-JIS")			},
	{	CPsfTags::CE_UTF8,			_T("UTF-8")				},
	{	NULL,						NULL					},
};

HHOOK	CMainWindow::g_messageFilterHook = NULL;
HWND	CMainWindow::g_messageFilterHookWindow = NULL;

CMainWindow::CMainWindow(CPsfVm& virtualMachine)
: Framework::Win32::CDialog(MAKEINTRESOURCE(IDD_MAINWINDOW))
, m_virtualMachine(virtualMachine)
, m_ready(false)
, m_frames(0)
, m_lastUpdateTime(~0)
, m_selectedAudioPlugin(DEFAULT_SOUND_HANDLER_ID)
, m_selectedCharEncoding(DEFAULT_CHAR_ENCODING_ID)
, m_timerLabel(NULL)
, m_titleLabel(NULL)
, m_placeHolder(NULL)
, m_configButton(NULL)
, m_pauseButton(NULL)
, m_repeatButton(NULL)
, m_playlistPanel(NULL)
, m_fileInformationPanel(NULL)
, m_spu0RegViewPanel(NULL)
, m_spu1RegViewPanel(NULL)
, m_currentPlaylistItem(0)
, m_repeatMode(DEFAULT_REPEAT_MODE)
, m_trackLength(0)
, m_accel(CreateAccelerators())
, m_reverbEnabled(true)
, m_discoveryThread(NULL)
, m_discoveryThreadActive(false)
, m_discoveryCommandQueue(5)
, m_discoveryResultQueue(5)
, m_discoveryRunId(0)
, m_playListOnceIcon(NULL)
, m_repeatListIcon(NULL)
, m_shuffleListIcon(NULL)
, m_repeatTrackIcon(NULL)
, m_toolTip(NULL)
, m_trayPopupMenu(NULL)
, m_configPopupMenu(NULL)
, m_useTrayIcon(false)
, m_trayIconServer(NULL)
, m_taskBarList(NULL)
, m_randomSeed(0)
{
	OSVERSIONINFO versionInfo;
	memset(&versionInfo, 0, sizeof(OSVERSIONINFO));
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInfo);
	if(versionInfo.dwMajorVersion > 6)
	{
		m_useTrayIcon = false;
	}
	else if(versionInfo.dwMajorVersion == 6)
	{
		m_useTrayIcon = versionInfo.dwMinorVersion < 1;
	}
	else
	{
		m_useTrayIcon = true;
	}
#ifdef FORCE_ENABLE_TRAYICON
	m_useTrayIcon = true;
#endif

	if(!m_useTrayIcon)
	{
		m_taskBarList = new Framework::Win32::CTaskBarList();
	}

	{
		srand(static_cast<unsigned int>(time(NULL)) * 0x13579ACD);
		float delta = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
		m_randomSeed = delta * static_cast<float>(0xCAFECAFE) + 0xDAEBB042;
	}

	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_REVERB_ENABLED, true);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_SOUNDHANDLER_ID, DEFAULT_SOUND_HANDLER_ID);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CHAR_ENCODING_ID, DEFAULT_CHAR_ENCODING_ID);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_REPEAT_MODE, DEFAULT_REPEAT_MODE);

	m_reverbEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_REVERB_ENABLED);
	m_repeatMode = static_cast<REPEAT_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_REPEAT_MODE));

	LoadAudioPluginPreferences();
	LoadCharEncodingPreferences();

	for(unsigned int i = 0; i < MAX_PANELS; i++)
	{
		m_panels[i] = NULL;
	}

	SetClassPtr();

	SetTimer(m_hWnd, TIMER_UPDATE_CLOCK, 200, NULL);
	SetTimer(m_hWnd, TIMER_UPDATE_FADE, 50, NULL);
	SetTimer(m_hWnd, TIMER_UPDATE_DISCOVERIES, 100, NULL);

	{
		int smallIconSizeX = GetSystemMetrics(SM_CXSMICON);
		int smallIconSizeY = GetSystemMetrics(SM_CYSMICON);
		int bigIconSizeX = GetSystemMetrics(SM_CXICON);
		int bigIconSizeY = GetSystemMetrics(SM_CYICON);

		HICON smallIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, smallIconSizeX, smallIconSizeY, 0));
		HICON bigIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, bigIconSizeX, bigIconSizeY, 0));

		SetIcon(ICON_SMALL, smallIcon);
		SetIcon(ICON_BIG, bigIcon);
	}

	m_trayPopupMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_POPUP));
	m_configPopupMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CONFIG_POPUP));

	m_virtualMachine.OnNewFrame.connect(std::tr1::bind(&CMainWindow::OnNewFrame, this));

	m_toolTip = new Framework::Win32::CToolTip(m_hWnd);
	m_toolTip->Activate(true);

	ChangeAudioPlugin(FindAudioPlugin(m_selectedAudioPlugin));

	m_playListOnceIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAYONCE), IMAGE_ICON, 16, 16, 0));
	m_repeatListIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_REPEAT_LIST), IMAGE_ICON, 16, 16, 0));
	m_shuffleListIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SHUFFLE_LIST), IMAGE_ICON, 16, 16, 0));
	m_repeatTrackIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_REPEAT_TRACK), IMAGE_ICON, 16, 16, 0));
	m_configIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_CONFIG), IMAGE_ICON, 16, 16, 0));
	m_playIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY), IMAGE_ICON, 16, 16, 0));
	m_pauseIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PAUSE), IMAGE_ICON, 16, 16, 0));
	m_prevTrackIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PREV_TRACK), IMAGE_ICON, 16, 16, 0));
	m_nextTrackIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NEXT_TRACK), IMAGE_ICON, 16, 16, 0));

	m_timerLabel = new Framework::Win32::CStatic(GetItem(IDC_TIMER_LABEL));
	m_titleLabel = new Framework::Win32::CStatic(GetItem(IDC_TITLE_LABEL));

	m_placeHolder = new Framework::Win32::CStatic(GetItem(IDC_PLACEHOLDER));

	m_configButton = new Framework::Win32::CButton(GetItem(IDC_CONFIG_BUTTON));
	m_toolTip->AddTool(m_configButton->m_hWnd, _T("Configuration"));
	m_configButton->SetIcon(m_configIcon);

	m_repeatButton = new Framework::Win32::CButton(GetItem(IDC_REPEAT_BUTTON));
	m_toolTip->AddTool(m_repeatButton->m_hWnd, _T(""));
	UpdateRepeatButton();

	m_pauseButton = new Framework::Win32::CButton(GetItem(IDC_PAUSE_BUTTON));

	//Initialize symbol fonts
	CreateSymbolFonts();

	{
		m_pauseButton->SetFont(m_webdingsFont);
		SendMessage(GetItem(ID_FILE_PREVIOUSTRACK), WM_SETFONT, reinterpret_cast<WPARAM>(static_cast<HFONT>(m_webdingsFont)), 0);
		SendMessage(GetItem(ID_FILE_NEXTTRACK), WM_SETFONT, reinterpret_cast<WPARAM>(static_cast<HFONT>(m_webdingsFont)), 0);
		SendMessage(GetItem(IDC_PREVTAB_BUTTON), WM_SETFONT, reinterpret_cast<WPARAM>(static_cast<HFONT>(m_webdingsFont)), 0);
		SendMessage(GetItem(IDC_NEXTTAB_BUTTON), WM_SETFONT, reinterpret_cast<WPARAM>(static_cast<HFONT>(m_webdingsFont)), 0);
	}

	if(m_segoeUiSymbolFont.IsValid())
	{
		SendMessage(GetItem(IDC_EJECT_BUTTON), WM_SETFONT, reinterpret_cast<WPARAM>(static_cast<HFONT>(m_segoeUiSymbolFont)), 0);
		SendMessage(GetItem(IDC_EJECT_BUTTON), WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(_T("\u23CF")));
	}

	//Create tray icon
	if(m_useTrayIcon)
	{
		m_trayIconServer = new Framework::Win32::CTrayIconServer();

		Framework::Win32::CTrayIcon* trayIcon = m_trayIconServer->Insert();
		trayIcon->SetIcon(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN)));
		trayIcon->SetTip(_T("PsfPlayer"));

		m_trayIconServer->RegisterHandler(std::tr1::bind(&CMainWindow::OnTrayIconEvent, this, 
			std::tr1::placeholders::_1, std::tr1::placeholders::_2));
	}

	//Create play list panel
	m_playlistPanel = new CPlaylistPanel(m_hWnd, m_playlist);
	m_playlistPanel->OnItemDblClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistItemDblClick, this, std::tr1::placeholders::_1));
	m_playlistPanel->OnAddClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistAddClick, this));
	m_playlistPanel->OnRemoveClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistRemoveClick, this, std::tr1::placeholders::_1));
	m_playlistPanel->OnSaveClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistSaveClick, this));

	//Create file information panel
	m_fileInformationPanel = new CFileInformationPanel(m_hWnd);

	//Create RegView panels
	m_spu0RegViewPanel = new CSpuRegViewPanel(m_hWnd, _T("SPU0"));
	m_spu1RegViewPanel = new CSpuRegViewPanel(m_hWnd, _T("SPU1"));

	//Initialize panels
	m_panels[0] = m_playlistPanel;
	m_panels[1] = m_fileInformationPanel;
	m_panels[2] = m_spu0RegViewPanel;
	m_panels[3] = m_spu1RegViewPanel;

	CreateAudioPluginMenu();
	UpdateAudioPluginMenu();

	CreateCharEncodingMenu();
	UpdateCharEncodingMenu();

	UpdateClock();
	UpdateTitle();
	UpdatePlaybackButtons();
	UpdateConfigMenu();

	m_currentPanel = -1;
	ActivatePanel(0);

	m_discoveryThreadActive = true;
	m_discoveryThread = new boost::thread(boost::bind(&CMainWindow::DiscoveryThreadProc, this));
}

CMainWindow::~CMainWindow()
{
	m_virtualMachine.Pause();

	delete m_timerLabel;
	delete m_titleLabel;
	delete m_placeHolder;
	delete m_configButton;
	delete m_repeatButton;
	delete m_pauseButton;

	if(m_discoveryThread)
	{
		m_discoveryThreadActive = false;
		m_discoveryThread->join();
		delete m_discoveryThread;
	}

	for(unsigned int i = 0; i < MAX_PANELS; i++)
	{
		if(m_panels[i] != NULL)
		{
			delete m_panels[i];
		}
	}

	delete m_toolTip;
	delete m_trayIconServer;
	delete m_taskBarList;

	DestroyIcon(m_playListOnceIcon);
	DestroyIcon(m_repeatListIcon);
	DestroyIcon(m_repeatTrackIcon);
	DestroyIcon(m_configIcon);
	DestroyIcon(m_playIcon);
	DestroyIcon(m_pauseIcon);
	DestroyIcon(m_prevTrackIcon);
	DestroyIcon(m_nextTrackIcon);
}

void CMainWindow::Run()
{
	while(IsWindow())
	{
		MSG msg;
		GetMessage(&msg, 0, 0, 0);
		if(!TranslateAccelerator(m_hWnd, m_accel, &msg))
		{
			if(!IsDialogMessage(m_hWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
}

CSoundHandler* CMainWindow::CreateHandler(const TCHAR* libraryPath)
{
	typedef CSoundHandler* (*HandlerFactoryFunction)();
	CSoundHandler* result = NULL;
	try
	{
		HMODULE module = LoadLibrary(libraryPath);
		if(module == NULL)
		{
			throw std::exception();
		}

		HandlerFactoryFunction handlerFactory = reinterpret_cast<HandlerFactoryFunction>(GetProcAddress(module, "HandlerFactory"));
		if(handlerFactory == NULL)
		{
			throw std::exception();
		}

		result = handlerFactory();
	}
	catch(...)
	{
		std::tstring errorMessage = _T("Couldn't create sound handler present in '") + std::tstring(libraryPath) + _T("'.");
		MessageBox(m_hWnd, errorMessage.c_str(), APP_NAME, 16);
	}
	return result;
}

long CMainWindow::OnWndProc(unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	if(m_taskBarList && (msg == m_taskBarList->GetCreatedMessage()))
	{
		static const int buttonCount = 3;
		static const UINT buttonIds[buttonCount] =
		{
			ID_FILE_PREVIOUSTRACK,
			ID_FILE_PAUSE,
			ID_FILE_NEXTTRACK
		};

		m_taskBarList->Initialize(m_hWnd);
		m_taskBarList->CreateThumbButtons(buttonCount, buttonIds);

		m_taskBarList->SetThumbButtonText(ID_FILE_PREVIOUSTRACK,	_T("Previous Track"));
		m_taskBarList->SetThumbButtonIcon(ID_FILE_PREVIOUSTRACK,	m_prevTrackIcon);

		m_taskBarList->SetThumbButtonText(ID_FILE_NEXTTRACK,		_T("Next Track"));
		m_taskBarList->SetThumbButtonIcon(ID_FILE_NEXTTRACK,		m_nextTrackIcon);

		UpdatePlaybackButtons();
	}
	return TRUE;
}

long CMainWindow::OnCommand(unsigned short nID, unsigned short nCmd, HWND hControl)
{
	if(!IsWindowEnabled(m_hWnd))
	{
		return TRUE;
	}

	switch(nID)
	{
	case ID_FILE_ABOUT:
		OnAbout();
		break;
	case ID_FILE_ENABLEREVERB:
		OnClickReverbEnabled();
		break;
	case ID_FILE_EXIT:
		DestroyWindow(m_hWnd);
		break;
	case ID_FILE_NEXTTRACK:
		OnNext();
		break;
	case ID_FILE_PREVIOUSTRACK:
		OnPrev();
		break;
	case IDC_PAUSE_BUTTON:
	case ID_FILE_PAUSE:
		OnPause();
		break;
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 0:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 1:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 2:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 3:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 4:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 5:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 6:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 7:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 8:
	case ID_FILE_AUDIOPLUGIN_PLUGIN_0 + 9:
		ChangeAudioPlugin(nID - ID_FILE_AUDIOPLUGIN_PLUGIN_0);
		break;
	case ID_FILE_CHARENCODING_ENCODING_0 + 0:
	case ID_FILE_CHARENCODING_ENCODING_0 + 1:
	case ID_FILE_CHARENCODING_ENCODING_0 + 2:
	case ID_FILE_CHARENCODING_ENCODING_0 + 3:
	case ID_FILE_CHARENCODING_ENCODING_0 + 4:
	case ID_FILE_CHARENCODING_ENCODING_0 + 5:
	case ID_FILE_CHARENCODING_ENCODING_0 + 6:
	case ID_FILE_CHARENCODING_ENCODING_0 + 7:
	case ID_FILE_CHARENCODING_ENCODING_0 + 8:
	case ID_FILE_CHARENCODING_ENCODING_0 + 9:
		ChangeCharEncoding(nID - ID_FILE_CHARENCODING_ENCODING_0);
		break;
	case IDC_CONFIG_BUTTON:
		OnConfig();
		break;
	case IDC_LOOPMODE_BUTTON:
		OnRepeat();
		break;
	case IDC_PREVTAB_BUTTON:
		OnPrevPanel();
		break;
	case IDC_NEXTTAB_BUTTON:
		OnNextPanel();
		break;
	case IDC_EJECT_BUTTON:
		OnFileOpen();
		break;
	}
	return TRUE;
}

long CMainWindow::OnTimer(WPARAM timerId)
{
	switch(timerId)
	{
	case TIMER_UPDATE_CLOCK:
		UpdateClock();
		break;
	case TIMER_UPDATE_FADE:
		UpdateFade();
		break;
	case TIMER_UPDATE_DISCOVERIES:
		ProcessPendingDiscoveries();
		break;
	}
	return TRUE;
}

long CMainWindow::OnClose()
{
	DestroyWindow(m_hWnd);
	return FALSE;
}

long CMainWindow::OnSize(unsigned int type, unsigned int width, unsigned int height)
{
	if(type == SIZE_MINIMIZED)
	{
		if(m_useTrayIcon)
		{
			Show(SW_HIDE);
		}
	}
	return TRUE;
}

void CMainWindow::OnPlaylistItemDblClick(unsigned int index)
{
	const auto& item(m_playlist.GetItem(index));
	
	boost::filesystem::path archivePath;
	if(item.archiveId != 0)
	{
		archivePath = m_playlist.GetArchive(item.archiveId);
	}

	if(PlayFile(item.path.c_str(), archivePath))
	{
		CPlaylist::ITEM newItem(item);
		CPlaylist::PopulateItemFromTags(newItem, m_tags);
		m_playlist.UpdateItem(index, newItem);
		m_currentPlaylistItem = index;
		m_playlistPanel->SetPlayingItemIndex(index);
	}
}

void CMainWindow::OnPlaylistAddClick()
{
	Framework::Win32::CFileDialog dialog(0x10000);
	const TCHAR* filter = 
		_T("All Supported Files\0*.psf; *.minipsf; *.psf2; *.minipsf2; *.psfp; *.minipsfp\0")
		PSF_FILTER
		PSF2_FILTER
		PSFP_FILTER;
	dialog.m_OFN.lpstrFilter = filter;
	dialog.m_OFN.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	if(dialog.SummonOpen(m_hWnd))
	{
		Framework::Win32::CFileDialog::PathList paths(dialog.GetMultiPaths());
		for(Framework::Win32::CFileDialog::PathList::const_iterator pathIterator(paths.begin());
			paths.end() != pathIterator; pathIterator++)
		{
			boost::filesystem::path filePath(*pathIterator);
			CPlaylist::ITEM item;
			item.path		= filePath.wstring();
			item.title		= filePath.wstring();
			item.length		= 0;
			unsigned int itemId = m_playlist.InsertItem(item);

			AddDiscoveryItem(filePath, boost::filesystem::path(), itemId);
		}
	}
}

void CMainWindow::OnPlaylistRemoveClick(unsigned int itemIdx)
{
	m_playlist.DeleteItem(itemIdx);
}

void CMainWindow::OnPlaylistSaveClick()
{
	Framework::Win32::CFileDialog dialog;
	const TCHAR* filter = PLAYLIST_FILTER;
	dialog.m_OFN.lpstrFilter = filter;
	dialog.m_OFN.lpstrDefExt = PLAYLIST_EXTENSION + 1;
	if(dialog.SummonSave(m_hWnd))
	{
		try
		{
			m_playlist.Write(dialog.GetPath());
		}
		catch(const std::exception& except)
		{
			std::tstring errorString = _T("Couldn't save playlist: \r\n\r\n");
			errorString += string_cast<std::tstring>(except.what());
			MessageBox(m_hWnd, errorString.c_str(), NULL, 16);
		}
	}
}

void CMainWindow::OnClickReverbEnabled()
{
	m_reverbEnabled = !m_reverbEnabled;
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_REVERB_ENABLED, m_reverbEnabled);
	UpdateConfigMenu();
	if(m_ready)
	{
		m_virtualMachine.SetReverbEnabled(m_reverbEnabled);
	}
}

void CMainWindow::OnTrayIconEvent(Framework::Win32::CTrayIcon* icon, LPARAM param)
{
	switch(param)
	{
	case WM_LBUTTONUP:
		Show(SW_SHOWNORMAL);
		SetForegroundWindow(m_hWnd);
		break;
	case WM_RBUTTONUP:
		DisplayTrayMenu();
		break;
	}
}

void CMainWindow::DisplayTrayMenu()
{
	POINT p;
	SetForegroundWindow(m_trayIconServer->m_hWnd);
	GetCursorPos(&p);
	TrackPopupMenu(GetSubMenu(m_trayPopupMenu, 0), TPM_RIGHTBUTTON, p.x, p.y, NULL, m_hWnd, NULL);
	PostMessage(m_hWnd, WM_NULL, 0, 0);
}

void CMainWindow::UpdateConfigMenu()
{
	Framework::Win32::CMenuItem reverbSubMenu(Framework::Win32::CMenuItem::FindById(m_configPopupMenu, ID_FILE_ENABLEREVERB));
	reverbSubMenu.Check(m_reverbEnabled);
}

void CMainWindow::UpdateFade()
{
	if(m_trackLength != 0)
	{
		if(m_frames >= m_trackLength)
		{
			unsigned int itemCount = m_playlist.GetItemCount();
			if((m_currentPlaylistItem + 1) == itemCount)
			{
				if(m_repeatMode == PLAYLIST_REPEAT)
				{
					m_currentPlaylistItem = 0;
					OnPlaylistItemDblClick(m_currentPlaylistItem);
				}
				else if(m_repeatMode == PLAYLIST_SHUFFLE)
				{
					OnNext();
				}
				else
				{
					Reset();
					UpdatePlaybackButtons();
				}
			}
			else
			{
				OnNext();
			}
		}
		else if(m_frames >= m_fadePosition)
		{
			float currentRatio = static_cast<float>(m_frames - m_fadePosition) / static_cast<float>(m_trackLength - m_fadePosition);
			float currentVolume = (1.0f - currentRatio) * m_volumeAdjust;
			m_virtualMachine.SetVolumeAdjust(currentVolume);
		}
	}
}

void CMainWindow::UpdateClock()
{
	static const unsigned int fps = 60;
	uint64 time = m_frames / fps;
	if(time != m_lastUpdateTime)
	{
		unsigned int seconds = static_cast<unsigned int>(time % 60);
		unsigned int minutes = static_cast<unsigned int>(time / 60);
		std::tstring timerText = lexical_cast_uint<std::tstring>(minutes, 2) + _T(":") + lexical_cast_uint<std::tstring>(seconds, 2);
		m_timerLabel->SetText(timerText.c_str());
		m_lastUpdateTime = time;
	}
}

void CMainWindow::UpdateTitle()
{
	std::tstring titleLabelText = APP_NAME;
	std::tstring windowText = APP_NAME;

	if(m_tags.HasTag("title"))
	{
		std::wstring titleTag = m_tags.GetTagValue("title");

		titleLabelText = string_cast<std::tstring>(titleTag);

		windowText += _T(" - [ ");
		windowText += string_cast<std::tstring>(titleTag);
		windowText += _T(" ]");
	}

	m_titleLabel->SetText(titleLabelText.c_str());
	SetText(windowText.c_str());
}

void CMainWindow::UpdatePlaybackButtons()
{
	CVirtualMachine::STATUS status = m_virtualMachine.GetStatus();
	if(m_pauseButton != NULL)
	{
		m_pauseButton->Enable(m_ready ? TRUE : FALSE);
		m_pauseButton->SetText((status == CVirtualMachine::PAUSED) ? TEXT_SYMBOL_PLAY : TEXT_SYMBOL_PAUSE);
	}
	if(m_configPopupMenu != NULL)
	{
		Framework::Win32::CMenuItem pauseMenu(Framework::Win32::CMenuItem::FindById(m_trayPopupMenu, ID_FILE_PAUSE));
		pauseMenu.Enable(m_ready ? TRUE : FALSE);
		pauseMenu.SetText((status == CVirtualMachine::PAUSED) ? TEXT_PLAY : TEXT_PAUSE);
	}
	if(m_taskBarList != NULL && m_taskBarList->IsInitialized())
	{
		m_taskBarList->SetThumbButtonText(ID_FILE_PAUSE, (status == CVirtualMachine::PAUSED) ? TEXT_PLAY : TEXT_PAUSE);
		m_taskBarList->SetThumbButtonIcon(ID_FILE_PAUSE, (status == CVirtualMachine::PAUSED) ? m_playIcon : m_pauseIcon);
		m_taskBarList->SetThumbButtonEnabled(ID_FILE_PAUSE, m_ready ? TRUE : FALSE);
	}
}

void CMainWindow::UpdateRepeatButton()
{
	switch(m_repeatMode)
	{
	default:
	case PLAYLIST_ONCE:
		m_repeatButton->SetIcon(m_playListOnceIcon);
		m_toolTip->SetToolText(reinterpret_cast<UINT_PTR>(m_repeatButton->m_hWnd), _T("Playlist Once"));
		break;
	case PLAYLIST_REPEAT:
		m_repeatButton->SetIcon(m_repeatListIcon);
		m_toolTip->SetToolText(reinterpret_cast<UINT_PTR>(m_repeatButton->m_hWnd), _T("Playlist Repeat"));
		break;
	case PLAYLIST_SHUFFLE:
		m_repeatButton->SetIcon(m_shuffleListIcon);
		m_toolTip->SetToolText(reinterpret_cast<UINT_PTR>(m_repeatButton->m_hWnd), _T("Playlist Shuffle"));
		break;
	case TRACK_REPEAT:
		m_repeatButton->SetIcon(m_repeatTrackIcon);
		m_toolTip->SetToolText(reinterpret_cast<UINT_PTR>(m_repeatButton->m_hWnd), _T("Track Repeat"));
		break;
	}
}

void CMainWindow::CreateAudioPluginMenu()
{
	Framework::Win32::CMenuItem pluginSubMenu(CreatePopupMenu());

	for(unsigned int i = 0; m_handlerInfo[i].name != NULL; i++)
	{
		std::tstring caption = m_handlerInfo[i].name;
		InsertMenu(pluginSubMenu, i, MF_STRING, ID_FILE_AUDIOPLUGIN_PLUGIN_0 + i, caption.c_str());
	}

	Framework::Win32::CMenuItem pluginMenu(Framework::Win32::CMenuItem::FindById(m_configPopupMenu, ID_FILE_AUDIOPLUGIN));
	MENUITEMINFO ItemInfo;
	memset(&ItemInfo, 0, sizeof(MENUITEMINFO));
	ItemInfo.cbSize		= sizeof(MENUITEMINFO);
	ItemInfo.fMask		= MIIM_SUBMENU;
	ItemInfo.hSubMenu	= pluginSubMenu;

	SetMenuItemInfo(pluginMenu, ID_FILE_AUDIOPLUGIN, FALSE, &ItemInfo);
}

void CMainWindow::UpdateAudioPluginMenu()
{
	for(unsigned int i = 0; m_handlerInfo[i].name != NULL; i++)
	{
		Framework::Win32::CMenuItem pluginSubMenuEntry(Framework::Win32::CMenuItem::FindById(m_configPopupMenu, ID_FILE_AUDIOPLUGIN_PLUGIN_0 + i));
		pluginSubMenuEntry.Check(m_handlerInfo[i].id == m_selectedAudioPlugin);
	}
}

void CMainWindow::LoadAudioPluginPreferences()
{
	int audioHandlerId = CAppConfig::GetInstance().GetPreferenceInteger(PREF_SOUNDHANDLER_ID);
	int audioHandlerIdx = FindAudioPlugin(audioHandlerId);
	if(audioHandlerIdx == -1)
	{
		m_selectedAudioPlugin = DEFAULT_SOUND_HANDLER_ID;
	}
	else
	{
		m_selectedAudioPlugin = audioHandlerId;
	}
}

int CMainWindow::FindAudioPlugin(unsigned int pluginId)
{
	for(unsigned int i = 0; m_handlerInfo[i].name != NULL; i++)
	{
		if(m_handlerInfo[i].id == pluginId) return i;
	}
	return -1;
}

void CMainWindow::ChangeAudioPlugin(unsigned int pluginIdx)
{
	SOUNDHANDLER_INFO* handlerInfo = m_handlerInfo + pluginIdx;
	m_selectedAudioPlugin = handlerInfo->id;
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_SOUNDHANDLER_ID, m_selectedAudioPlugin);
	m_virtualMachine.SetSpuHandler(std::tr1::bind(&CMainWindow::CreateHandler, this, handlerInfo->dllName));
	UpdateAudioPluginMenu();
}

void CMainWindow::CreateCharEncodingMenu()
{
	Framework::Win32::CMenuItem pluginSubMenu(CreatePopupMenu());

	for(unsigned int i = 0; m_charEncodingInfo[i].name != NULL; i++)
	{
		std::tstring caption = m_charEncodingInfo[i].name;
		InsertMenu(pluginSubMenu, i, MF_STRING, ID_FILE_CHARENCODING_ENCODING_0 + i, caption.c_str());
	}

	Framework::Win32::CMenuItem pluginMenu(Framework::Win32::CMenuItem::FindById(m_configPopupMenu, ID_FILE_CHARACTERENCODING));
	MENUITEMINFO ItemInfo;
	memset(&ItemInfo, 0, sizeof(MENUITEMINFO));
	ItemInfo.cbSize		= sizeof(MENUITEMINFO);
	ItemInfo.fMask		= MIIM_SUBMENU;
	ItemInfo.hSubMenu	= pluginSubMenu;

	SetMenuItemInfo(pluginMenu, ID_FILE_CHARACTERENCODING, FALSE, &ItemInfo);
}

void CMainWindow::UpdateCharEncodingMenu()
{
	for(unsigned int i = 0; m_charEncodingInfo[i].name != NULL; i++)
	{
		Framework::Win32::CMenuItem pluginSubMenuEntry(Framework::Win32::CMenuItem::FindById(m_configPopupMenu, ID_FILE_CHARENCODING_ENCODING_0 + i));
		pluginSubMenuEntry.Check(m_charEncodingInfo[i].id == m_selectedCharEncoding);
	}
}

void CMainWindow::ChangeCharEncoding(unsigned int encodingIdx)
{
	CHARENCODING_INFO* encodingInfo = m_charEncodingInfo + encodingIdx;
	m_selectedCharEncoding = encodingInfo->id;
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CHAR_ENCODING_ID, m_selectedCharEncoding);
	UpdateCharEncodingMenu();
}

void CMainWindow::LoadCharEncodingPreferences()
{
	int charEncodingId = CAppConfig::GetInstance().GetPreferenceInteger(PREF_CHAR_ENCODING_ID);
	int charEncodingIdx = FindCharEncoding(charEncodingId);
	if(charEncodingIdx == -1)
	{
		m_selectedCharEncoding = DEFAULT_CHAR_ENCODING_ID;
	}
	else
	{
		m_selectedCharEncoding = charEncodingId;
	}
}

int CMainWindow::FindCharEncoding(unsigned int encodingId)
{
	for(unsigned int i = 0; m_charEncodingInfo[i].name != NULL; i++)
	{
		if(m_charEncodingInfo[i].id == encodingId) return i;
	}
	return -1;
}

HACCEL CMainWindow::CreateAccelerators()
{
	Framework::Win32::CAcceleratorTableGenerator tableGenerator;
	tableGenerator.Insert(ID_FILE_PAUSE,			VK_F5,	FVIRTKEY);
	tableGenerator.Insert(ID_FILE_ENABLEREVERB,		'R',	FVIRTKEY | FCONTROL);
	return tableGenerator.Create();
}

void CMainWindow::CreateSymbolFonts()
{
	HFONT baseFont = m_pauseButton->GetFont();

	{
		LOGFONT fontDef = { 0 };
		GetObject(baseFont, sizeof(LOGFONT), &fontDef);
		fontDef.lfHeight			= MulDiv(fontDef.lfHeight, 4, 3);
		fontDef.lfWeight			= FW_NORMAL;
		fontDef.lfCharSet			= DEFAULT_CHARSET;
		fontDef.lfOutPrecision		= OUT_DEFAULT_PRECIS;
		fontDef.lfClipPrecision		= CLIP_DEFAULT_PRECIS;
		fontDef.lfQuality			= DEFAULT_QUALITY;
		fontDef.lfPitchAndFamily	= DEFAULT_PITCH;
		_tcscpy(fontDef.lfFaceName, _T("Webdings"));
		m_webdingsFont = Framework::Win32::CFont(CreateFontIndirect(&fontDef));
	}

	{
		LOGFONT fontDef = { 0 };
		GetObject(baseFont, sizeof(LOGFONT), &fontDef);
		fontDef.lfHeight			= MulDiv(fontDef.lfHeight, 4, 3);
		fontDef.lfWeight			= FW_NORMAL;
		fontDef.lfCharSet			= DEFAULT_CHARSET;
		fontDef.lfOutPrecision		= OUT_DEFAULT_PRECIS;
		fontDef.lfClipPrecision		= CLIP_DEFAULT_PRECIS;
		fontDef.lfQuality			= DEFAULT_QUALITY;
		fontDef.lfPitchAndFamily	= DEFAULT_PITCH;
		_tcscpy(fontDef.lfFaceName, _T("Segoe UI Symbol"));
		m_segoeUiSymbolFont = Framework::Win32::CFont(CreateFontIndirect(&fontDef));
	}
}

uint32 CMainWindow::GetPrevRandomNumber(uint32 seed)
{
	uint32 msb = (seed & 0x80000000) ? 1 : 0;
	seed <<= 1;
	seed |= msb;
	seed = (seed - 0xCAFECAFE) ^ 0xDEADBEEF;
	return seed;
}

uint32 CMainWindow::GetNextRandomNumber(uint32 seed)
{
	seed = (seed ^ 0xDEADBEEF) + 0xCAFECAFE;
	uint32 lsb = seed & 1;
	seed >>= 1;
	seed |= (lsb << 31);
	return seed;
}

void CMainWindow::OnNewFrame()
{
	m_frames++;
}

void CMainWindow::OnFileOpen()
{
	Framework::Win32::CFileDialog dialog;
	const TCHAR* filter = 
		_T("All Supported Files\0*.zip; *.rar; *.psf; *.minipsf; *.psf2; *.minipsf2; *.psfp; *.minipsfp;*") PLAYLIST_EXTENSION _T("\0")
		PLAYLIST_FILTER
		ARCHIVE_FILTER
		PSF_FILTER
		PSF2_FILTER
		PSFP_FILTER;
	dialog.m_OFN.lpstrFilter = filter;
	if(dialog.SummonOpen(m_hWnd))
	{
		boost::filesystem::path filePath(dialog.GetPath());
		std::wstring fileExtension = filePath.extension().wstring();
		if(!wcsicmp(fileExtension.c_str(), PLAYLIST_EXTENSION))
		{
			LoadPlaylist(filePath);
		}
		else if(!wcsicmp(fileExtension.c_str(), L".rar") || !wcsicmp(fileExtension.c_str(), L".zip"))
		{
			LoadArchive(filePath);
		}
		else
		{
			LoadSingleFile(filePath);
		}
	}
}

void CMainWindow::OnPause()
{
	if(!m_ready) return;
	if(m_virtualMachine.GetStatus() == CVirtualMachine::PAUSED)
	{
		m_virtualMachine.Resume();
	}
	else
	{
		m_virtualMachine.Pause();
	}
	UpdatePlaybackButtons();
}

void CMainWindow::OnPrev()
{
	if(m_playlist.GetItemCount() == 0) return;
	if(m_repeatMode == PLAYLIST_SHUFFLE)
	{
		unsigned int itemCount = m_playlist.GetItemCount();
		m_randomSeed = GetPrevRandomNumber(m_randomSeed);
		m_currentPlaylistItem = (m_randomSeed % itemCount);
	}
	else
	{
		if(m_currentPlaylistItem != 0)
		{
			m_currentPlaylistItem--;
		}
	}
	OnPlaylistItemDblClick(m_currentPlaylistItem);
}

void CMainWindow::OnNext()
{
	if(m_playlist.GetItemCount() == 0) return;
	unsigned int itemCount = m_playlist.GetItemCount();
	if(m_repeatMode == PLAYLIST_SHUFFLE)
	{
		m_randomSeed = GetNextRandomNumber(m_randomSeed);
		m_currentPlaylistItem = (m_randomSeed % itemCount);
	}
	else
	{
		if((m_currentPlaylistItem + 1) < itemCount)
		{
			m_currentPlaylistItem++;
		}
	}
	OnPlaylistItemDblClick(m_currentPlaylistItem);
}

void CMainWindow::OnPrevPanel()
{
	unsigned int newPanelIdx = (m_currentPanel == 0) ? (MAX_PANELS - 1) : (m_currentPanel - 1);
	ActivatePanel(newPanelIdx);
}

void CMainWindow::OnNextPanel()
{
	unsigned int newPanelIdx = (m_currentPanel == MAX_PANELS - 1) ? 0 : (m_currentPanel + 1);
	ActivatePanel(newPanelIdx);
}

void CMainWindow::OnRepeat()
{
	switch(m_repeatMode)
	{
	default:
	case PLAYLIST_ONCE:
		m_repeatMode = PLAYLIST_REPEAT;
		break;
	case PLAYLIST_REPEAT:
		m_repeatMode = PLAYLIST_SHUFFLE;
		break;
	case PLAYLIST_SHUFFLE:
		m_repeatMode = TRACK_REPEAT;
		break;
	case TRACK_REPEAT:
		m_repeatMode = PLAYLIST_ONCE;
		break;
	}
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_REPEAT_MODE, m_repeatMode);
	UpdateRepeatButton();
}

void CMainWindow::OnConfig()
{
	SetForegroundWindow(m_hWnd);

	RECT buttonRect = m_configButton->GetClientRect();
	ClientToScreen(m_configButton->m_hWnd, reinterpret_cast<POINT*>(&buttonRect) + 0);
	ClientToScreen(m_configButton->m_hWnd, reinterpret_cast<POINT*>(&buttonRect) + 1);

	TPMPARAMS tpmParams;
	memset(&tpmParams, 0, sizeof(tpmParams));
	tpmParams.cbSize = sizeof(tpmParams);
	tpmParams.rcExclude = buttonRect;

	g_messageFilterHookWindow = m_configButton->m_hWnd;
	g_messageFilterHook = SetWindowsHookEx(WH_MSGFILTER, MessageHookProc, NULL, GetCurrentThreadId());
	TrackPopupMenuEx(GetSubMenu(m_configPopupMenu, 0), TPM_VERTICAL, buttonRect.left, buttonRect.top, m_hWnd, &tpmParams);
	UnhookWindowsHookEx(g_messageFilterHook);
	g_messageFilterHook = NULL;

	m_configButton->SetCheck(false);
}

LRESULT CALLBACK CMainWindow::MessageHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	auto message = reinterpret_cast<MSG*>(lParam);
	if(message->message == WM_LBUTTONDOWN)
	{
		HWND clickedWindow = WindowFromPoint(message->pt);
		if(clickedWindow == g_messageFilterHookWindow)
		{
			PostMessage(message->hwnd, WM_CANCELMODE, 0, 0);
			return TRUE;
		}
	}
	return CallNextHookEx(g_messageFilterHook, code, wParam, lParam);
}

void CMainWindow::ActivatePanel(unsigned int panelIdx)
{
	if(m_currentPanel != -1)
	{
		m_panels[m_currentPanel]->Show(SW_HIDE);
	}
	RECT placeHolderRect;
	m_placeHolder->GetWindowRect(&placeHolderRect);
	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&placeHolderRect) + 0);
	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&placeHolderRect) + 1);
	m_currentPanel = panelIdx;
	m_panels[m_currentPanel]->SetSizePosition(&placeHolderRect);
	SetWindowPos(m_panels[m_currentPanel]->m_hWnd, GetItem(IDC_NEXTTAB_BUTTON), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	m_panels[m_currentPanel]->Show(SW_SHOW);
}

void CMainWindow::OnAbout()
{
	CAboutWindow aboutWindow;
	aboutWindow.DoModal(m_hWnd);
}

void CMainWindow::LoadSingleFile(const boost::filesystem::path& filePath)
{
	m_playlist.Clear();
	ResetDiscoveryRun();

	{
		CPlaylist::ITEM item;
		item.path = filePath.wstring();
		CPlaylist::PopulateItemFromTags(item, m_tags);
		m_playlist.InsertItem(item);
	}

	m_currentPlaylistItem = 0;
	OnPlaylistItemDblClick(m_currentPlaylistItem);
}

void CMainWindow::LoadPlaylist(const boost::filesystem::path& playlistPath)
{
	try
	{
		m_playlist.Clear();
		ResetDiscoveryRun();

		m_playlist.Read(playlistPath);
		if(m_playlist.GetItemCount() > 0)
		{
			if(m_repeatMode == PLAYLIST_SHUFFLE)
			{
				unsigned int itemCount = m_playlist.GetItemCount();
				m_randomSeed = GetNextRandomNumber(m_randomSeed);
				m_currentPlaylistItem = (m_randomSeed % itemCount);
			}
			else
			{
				m_currentPlaylistItem = 0;
			}
			OnPlaylistItemDblClick(m_currentPlaylistItem);
		}

		for(unsigned int i = 0; i < m_playlist.GetItemCount(); i++)
		{
			const CPlaylist::ITEM& item(m_playlist.GetItem(i));
			AddDiscoveryItem(item.path.c_str(), boost::filesystem::path(), item.id);
		}
	}
	catch(const std::exception& except)
	{
		std::tstring errorString = _T("Couldn't load playlist: \r\n\r\n");
		errorString += string_cast<std::tstring>(except.what());
		MessageBox(m_hWnd, errorString.c_str(), NULL, 16);
	}
}

void CMainWindow::LoadArchive(const boost::filesystem::path& archivePath)
{
	try
	{
		m_playlist.Clear();
		ResetDiscoveryRun();

		{
			boost::scoped_ptr<CPsfArchive> archive(CPsfArchive::CreateFromPath(archivePath));
			unsigned int archiveId = m_playlist.InsertArchive(archivePath.wstring().c_str());

			for(CPsfArchive::FileListIterator fileIterator(archive->GetFilesBegin());
				fileIterator != archive->GetFilesEnd(); fileIterator++)
			{
				boost::filesystem::path filePath(fileIterator->name);
				std::string fileExtension = filePath.extension().string();
				if((fileExtension.length() != 0) && CPlaylist::IsLoadableExtension(fileExtension.c_str() + 1))
				{
					CPlaylist::ITEM newItem;
					newItem.path = filePath.wstring();
					newItem.title = newItem.path;
					newItem.length = 0;
					newItem.archiveId = archiveId;
					unsigned int itemId = m_playlist.InsertItem(newItem);

					AddDiscoveryItem(filePath, archivePath, itemId);
				}
			}
		}

		if(m_playlist.GetItemCount() > 0)
		{
			if(m_repeatMode == PLAYLIST_SHUFFLE)
			{
				unsigned int itemCount = m_playlist.GetItemCount();
				m_randomSeed = GetNextRandomNumber(m_randomSeed);
				m_currentPlaylistItem = (m_randomSeed % itemCount);
			}
			else
			{
				m_currentPlaylistItem = 0;
			}
			OnPlaylistItemDblClick(m_currentPlaylistItem);
		}
	}
	catch(const std::exception& except)
	{
		std::tstring errorString = _T("Couldn't load archive: \r\n\r\n");
		errorString += string_cast<std::tstring>(except.what());
		MessageBox(m_hWnd, errorString.c_str(), NULL, 16);
	}
}

void CMainWindow::Reset()
{
	m_spu0RegViewPanel->SetSpu(NULL);
	m_spu1RegViewPanel->SetSpu(NULL);
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();
	m_frames = 0;
	m_lastUpdateTime = ~0;
	m_trackLength = 0;
	m_fadePosition = 0;
	m_ready = false;
	m_volumeAdjust = 1.0f;
}

bool CMainWindow::PlayFile(const boost::filesystem::path& filePath, const boost::filesystem::path& archivePath)
{
	EnableWindow(m_hWnd, FALSE);

	Reset();
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(m_virtualMachine, filePath, archivePath, &tags);
		m_tags = CPsfTags(tags);
		m_tags.SetDefaultCharEncoding(static_cast<CPsfTags::CHAR_ENCODING>(m_selectedCharEncoding));
		try
		{
			m_volumeAdjust = boost::lexical_cast<float>(m_tags.GetTagValue("volume"));
			m_virtualMachine.SetVolumeAdjust(m_volumeAdjust);
		}
		catch(...)
		{

		}
		m_fileInformationPanel->SetTags(m_tags);
		m_spu0RegViewPanel->SetSpu(&m_virtualMachine.GetSpuCore(0));
		m_spu1RegViewPanel->SetSpu(&m_virtualMachine.GetSpuCore(1));
		m_virtualMachine.SetReverbEnabled(m_reverbEnabled);
		m_virtualMachine.Resume();
		m_ready = true;
		if(m_repeatMode != TRACK_REPEAT)
		{
			double fade = 10;
			if(m_tags.HasTag("length"))
			{
				double length = CPsfTags::ConvertTimeString(m_tags.GetTagValue("length").c_str());
				m_trackLength = static_cast<uint64>(length * 60.0);
			}
			if(m_tags.HasTag("fade"))
			{
				fade = CPsfTags::ConvertTimeString(m_tags.GetTagValue("fade").c_str());
			}
			m_fadePosition = m_trackLength;
			m_trackLength +=  static_cast<uint64>(fade * 60.0);
		}
	}
	catch(const std::exception& except)
	{
		std::tstring errorString = _T("Couldn't load PSF file: \r\n\r\n");
		errorString += string_cast<std::tstring>(except.what());
		MessageBox(m_hWnd, errorString.c_str(), NULL, 16);
	}

	EnableWindow(m_hWnd, TRUE);

	UpdateTitle();
	UpdatePlaybackButtons();
	UpdateClock();

	return m_ready;
}

void CMainWindow::AddDiscoveryItem(const boost::filesystem::path& filePath, const boost::filesystem::path& archivePath, unsigned int itemId)
{
	DISCOVERY_COMMAND command;
	command.runId		= m_discoveryRunId;
	command.itemId		= itemId;
	command.filePath	= filePath;
	command.archivePath	= archivePath;
	m_pendingDiscoveryCommands.push_back(command);
}

void CMainWindow::ResetDiscoveryRun()
{
	m_pendingDiscoveryCommands.clear();
	m_discoveryRunId++;
}

void CMainWindow::ProcessPendingDiscoveries()
{
	while(m_pendingDiscoveryCommands.size() != 0)
	{
		const DISCOVERY_COMMAND& command(*m_pendingDiscoveryCommands.begin());
		if(m_discoveryCommandQueue.TryPush(command))
		{
			m_pendingDiscoveryCommands.pop_front();
		}
		else
		{
			break;
		}
	}

	{
		DISCOVERY_RESULT result;
		if(m_discoveryResultQueue.TryPop(result))
		{
			if(result.runId == m_discoveryRunId)
			{
				int itemIdx = m_playlist.FindItem(result.itemId);
				if(itemIdx != -1)
				{
					CPlaylist::ITEM item = m_playlist.GetItem(itemIdx);
					item.title	= result.title;
					item.length	= result.length;
					m_playlist.UpdateItem(itemIdx, item);
				}
			}
		}
	}
}

void CMainWindow::DiscoveryThreadProc()
{
	DiscoveryResultQueue pendingResults;

	while(m_discoveryThreadActive)
	{
		DISCOVERY_COMMAND command;
		if(m_discoveryCommandQueue.TryPop(command))
		{
			try
			{
				boost::scoped_ptr<CPsfStreamProvider> streamProvider(CreatePsfStreamProvider(command.archivePath));
				boost::scoped_ptr<Framework::CStream> inputStream(streamProvider->GetStreamForPath(command.filePath));
				CPsfBase psfFile(*inputStream);
				CPsfTags tags(CPsfTags::TagMap(psfFile.GetTagsBegin(), psfFile.GetTagsEnd()));
				tags.SetDefaultCharEncoding(static_cast<CPsfTags::CHAR_ENCODING>(m_selectedCharEncoding));

				DISCOVERY_RESULT result;
				result.runId = command.runId;
				result.itemId = command.itemId;
				result.length = 0;

				if(tags.HasTag("title"))
				{
					result.title = tags.GetTagValue("title");
				}
				if(tags.HasTag("length"))
				{
					result.length = tags.ConvertTimeString(tags.GetTagValue("length").c_str());
				}

				pendingResults.push_back(result);
			}
			catch(...)
			{
				//assert(0);
			}
		}

		while(pendingResults.size() != 0)
		{
			const DISCOVERY_RESULT& command(*pendingResults.begin());
			if(m_discoveryResultQueue.TryPush(command))
			{
				pendingResults.pop_front();
			}
			else
			{
				break;
			}
		}

		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}
}
