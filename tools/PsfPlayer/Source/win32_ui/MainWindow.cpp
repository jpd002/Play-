#include "AppDef.h"
#include "AppConfig.h"
#include "MainWindow.h"
#include "AboutWindow.h"
#include "../PsfLoader.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "win32/MenuItem.h"
#include "win32/AcceleratorTableGenerator.h"
#include "layout/LayoutEngine.h"
#include "placeholder_def.h"
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "resource.h"
#include <afxres.h>
#include "stricmp.h"

#define CLSNAME			                _T("MainWindow")
#define WNDSTYLE		                (WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX)
#define WNDSTYLEEX		                (0)

#define ID_FILE_AUDIOPLUGIN_PLUGIN_0    (0xBEEF)
#define ID_FILE_CHARENCODING_ENCODING_0	(0xCEEF)

#define PLAYLIST_EXTENSION              _T("psfpl")
#define PLAYLIST_FILTER                 _T("PsfPlayer Playlists (*.") PLAYLIST_EXTENSION _T(")\0*.") PLAYLIST_EXTENSION _T("\0")
#define PSF_FILTER						_T("PlayStation Sound Files (*.psf; *.minipsf)\0*.psf; *.minipsf\0")
#define PSF2_FILTER						_T("PlayStation2 Sound Files (*.psf2; *.minipsf2)\0*.psf2; *.minipsf2\0")
#define PSFP_FILTER						_T("PlayStation Portable Sound Files (*.psfp; *.minipsfp)\0*.psfp; *.minipsfp\0")

#define PREF_SOUNDHANDLER_ID			("soundhandler.id")
#define DEFAULT_SOUND_HANDLER_ID		(1)

#define PREF_CHAR_ENCODING_ID			("charencoding.id")
#define DEFAULT_CHAR_ENCODING_ID		(CPsfTags::CE_WINDOWS_1252)

CMainWindow::SOUNDHANDLER_INFO CMainWindow::m_handlerInfo[] =
{
    {   1,      _T("Win32 WaveOut"),    _T("SH_WaveOut.dll")    },
    {   2,      _T("OpenAL"),           _T("SH_OpenAL.dll")     },
    {   NULL,   NULL,                   NULL                    },
};

CMainWindow::CHARENCODING_INFO CMainWindow::m_charEncodingInfo[] =
{
	{	CPsfTags::CE_WINDOWS_1252,	_T("Windows 1252")		},
	{	CPsfTags::CE_SHIFT_JIS,		_T("Shift-JIS")			},
	{	CPsfTags::CE_UTF8,			_T("UTF-8")				},
    {   NULL,						NULL                    },
};

using namespace Framework;
using namespace std;

CMainWindow::CMainWindow(CPsfVm& virtualMachine) :
m_virtualMachine(virtualMachine),
m_ready(false),
m_frames(0),
m_selectedAudioPlugin(DEFAULT_SOUND_HANDLER_ID),
m_selectedCharEncoding(DEFAULT_CHAR_ENCODING_ID),
m_ejectButton(NULL),
m_pauseButton(NULL),
m_repeatButton(NULL),
m_playlistPanel(NULL),
m_fileInformationPanel(NULL),
m_spu0RegViewPanel(NULL),
m_spu1RegViewPanel(NULL),
m_nextPanelButton(NULL),
m_prevPanelButton(NULL),
m_currentPlaylistItem(0),
m_repeatMode(PLAYLIST_ONCE),
m_trackLength(0),
m_accel(CreateAccelerators())
{
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_SOUNDHANDLER_ID, DEFAULT_SOUND_HANDLER_ID);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CHAR_ENCODING_ID, DEFAULT_CHAR_ENCODING_ID);

	LoadAudioPluginPreferences();
	LoadCharEncodingPreferences();

	for(unsigned int i = 0; i < MAX_PANELS; i++)
    {
        m_panels[i] = NULL;
    }

    if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&Win32::CWindow::MakeWndClass(CLSNAME));
	}

    Win32::CRect clientRect(0, 0, 320, 480);
    AdjustWindowRectEx(clientRect, WNDSTYLE, FALSE, WNDSTYLEEX);
    Create(WNDSTYLEEX, CLSNAME, APP_NAME, WNDSTYLE, clientRect, NULL, NULL);
    SetClassPtr();

	SetTimer(m_hWnd, TIMER_UPDATE_CLOCK, 200, NULL);
	SetTimer(m_hWnd, TIMER_UPDATE_FADE, 50, NULL);

	SetIcon(ICON_SMALL, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN)));
    m_popupMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_POPUP));

    m_virtualMachine.OnNewFrame.connect(std::tr1::bind(&CMainWindow::OnNewFrame, this));

    ChangeAudioPlugin(FindAudioPlugin(m_selectedAudioPlugin));

    m_timerLabel = new Win32::CStatic(m_hWnd, _T(""), SS_CENTER);
    m_titleLabel = new Win32::CStatic(m_hWnd, _T(""), SS_CENTER | SS_NOPREFIX);

    m_placeHolder = new Win32::CStatic(m_hWnd, Win32::CRect(0, 0, 1, 1));

	m_nextPanelButton = new Win32::CButton(_T(">"), m_hWnd, Win32::CRect(0, 0, 1, 1));
	m_prevPanelButton = new Win32::CButton(_T("<"), m_hWnd, Win32::CRect(0, 0, 1, 1));
	m_repeatButton = new Win32::CButton(_T("Repeat"), m_hWnd, Win32::CRect(0, 0, 1, 1));
	UpdateRepeatButton();

	m_nextButton = new Win32::CButton(_T(">>"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_prevButton = new Win32::CButton(_T("<<"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_pauseButton = new Win32::CButton(_T("Pause"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_ejectButton = new Win32::CButton(_T("Eject"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    
    //m_aboutButton = new Win32::CButton(_T("About"), m_hWnd, Win32::CRect(0, 0, 1, 1));

    m_layout = 
        VerticalLayoutContainer(
            LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_titleLabel)) +
            LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_timerLabel)) +
            HorizontalLayoutContainer(
                LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(50, 20, m_prevPanelButton)) +
                LayoutExpression(CLayoutStretch::Create()) +
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(100, 20, m_repeatButton)) +
                LayoutExpression(CLayoutStretch::Create()) +
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(50, 20, m_nextPanelButton))
            ) +
			LayoutExpression(Win32::CLayoutWindow::CreateCustomBehavior(300, 200, 1, 1, m_placeHolder)) +
            HorizontalLayoutContainer(
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_prevButton)) +
                LayoutExpression(CLayoutStretch::Create()) +
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_pauseButton)) +
                LayoutExpression(CLayoutStretch::Create()) +
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_nextButton))
            ) +
            HorizontalLayoutContainer(
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_ejectButton))
//                LayoutExpression(CLayoutStretch::Create()) +
//                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_aboutButton))
            )
        );

    //Create tray icon
    Win32::CTrayIcon* trayIcon = m_trayIconServer.Insert();
    trayIcon->SetIcon(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN)));
    trayIcon->SetTip(_T("PsfPlayer"));

    m_trayIconServer.RegisterHandler(std::tr1::bind(&CMainWindow::OnTrayIconEvent, this, 
        std::tr1::placeholders::_1, std::tr1::placeholders::_2));

    //Create play list panel
    m_playlistPanel = new CPlaylistPanel(m_placeHolder->m_hWnd, m_playlist);
    m_playlistPanel->OnItemDblClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistItemDblClick, this, std::tr1::placeholders::_1));
    m_playlistPanel->OnAddClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistAddClick, this));
    m_playlistPanel->OnRemoveClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistRemoveClick, this, std::tr1::placeholders::_1));
    m_playlistPanel->OnSaveClick.connect(std::tr1::bind(&CMainWindow::OnPlaylistSaveClick, this));

	//Create file information panel
	m_fileInformationPanel = new CFileInformationPanel(m_placeHolder->m_hWnd);

	//Create RegView panels
	m_spu0RegViewPanel = new CSpuRegViewPanel(m_placeHolder->m_hWnd, _T("SPU0"));
	m_spu1RegViewPanel = new CSpuRegViewPanel(m_placeHolder->m_hWnd, _T("SPU1"));

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
    UpdateButtons();
	RefreshLayout();

	m_currentPanel = -1;
	ActivatePanel(0);
}

CMainWindow::~CMainWindow()
{
	m_virtualMachine.Pause();

	for(unsigned int i = 0; i < MAX_PANELS; i++)
	{
		if(m_panels[i] != NULL)
		{
			delete m_panels[i];
			m_panels[i] = NULL;
		}
	}
}

void CMainWindow::Run()
{
    while(IsWindow())
    {
        MSG msg;
        GetMessage(&msg, 0, 0, 0);
        if(!TranslateAccelerator(m_hWnd, m_accel, &msg))
        {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
        }
    }
}

void CMainWindow::RefreshLayout()
{
	RECT rc = GetClientRect();
	m_layout->SetRect(rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);
	m_layout->RefreshGeometry();

    for(unsigned int i = 0; i < MAX_PANELS; i++)
    {
        CPanel* panel(m_panels[i]);
        if(panel != NULL)
        {
            panel->RefreshLayout();
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
        tstring errorMessage = _T("Couldn't create sound handler present in '") + tstring(libraryPath) + _T("'.");
        MessageBox(m_hWnd, errorMessage.c_str(), APP_NAME, 16);
    }
	return result;
}

long CMainWindow::OnCommand(unsigned short nID, unsigned short nCmd, HWND hControl)
{
    if(CWindow::IsCommandSource(m_ejectButton, hControl))
    {
        OnFileOpen();
    }
    else if(CWindow::IsCommandSource(m_pauseButton, hControl))
    {
        OnPause();
    }
    else if(CWindow::IsCommandSource(m_nextButton, hControl))
    {
        OnNext();
    }
    else if(CWindow::IsCommandSource(m_prevButton, hControl))
    {
        OnPrev();
    }
	else if(CWindow::IsCommandSource(m_nextPanelButton, hControl))
	{
		OnNextPanel();
	}
	else if(CWindow::IsCommandSource(m_prevPanelButton, hControl))
	{
		OnPrevPanel();
	}
	else if(CWindow::IsCommandSource(m_repeatButton, hControl))
	{
		OnRepeat();
	}
    else
    {
	    switch(nID)
	    {
        case ID_FILE_ABOUT:
            OnAbout();
            break;
        case ID_FILE_EXIT:
		    DestroyWindow(m_hWnd);
            break;
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
	    }
    }
	return TRUE;    
}

long CMainWindow::OnTimer(WPARAM timerId)
{
	if(timerId == TIMER_UPDATE_CLOCK)
	{
		UpdateClock();
	}
	else if(timerId == TIMER_UPDATE_FADE)
	{
		UpdateFade();
	}
    return TRUE;
}

//long CMainWindow::OnSysCommand(unsigned int nCmd, LPARAM lParam)
//{
//	switch(nCmd)
//	{
//	case SC_CLOSE:
//		//if(!m_nPopUpVisible) break;
//		Show(SW_HIDE);
//		return FALSE;
//	}
//	return TRUE;
//}

long CMainWindow::OnSize(unsigned int type, unsigned int width, unsigned int height)
{
    if(type == SIZE_MINIMIZED)
    {
        Show(SW_HIDE);
    }
    return TRUE;
}

void CMainWindow::OnPlaylistItemDblClick(unsigned int index)
{
    const CPlaylist::ITEM& item(m_playlist.GetItem(index));
    if(PlayFile(item.path.c_str()))
    {
        CPlaylist::ITEM newItem(item);
        CPlaylist::PopulateItemFromTags(newItem, m_tags);
        m_playlist.UpdateItem(index, newItem);
        m_currentPlaylistItem = index;
    }
}

void CMainWindow::OnPlaylistAddClick()
{
    Win32::CFileDialog dialog(0x10000);
    const TCHAR* filter = 
	    _T("All Supported Files\0*.psf; *.minipsf; *.psf2; *.minipsf2; *.psfp; *.minipsfp\0")
	    PSF_FILTER
	    PSF2_FILTER
		PSFP_FILTER;
    dialog.m_OFN.lpstrFilter = filter;
    dialog.m_OFN.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    if(dialog.SummonOpen(m_hWnd))
    {
        Win32::CFileDialog::PathList paths(dialog.GetMultiPaths());
        for(Win32::CFileDialog::PathList::const_iterator pathIterator(paths.begin());
            paths.end() != pathIterator; pathIterator++)
        {
            const std::tstring path(*pathIterator);
            CPlaylist::ITEM item;
            item.path       = string_cast<string>(path);
            item.title      = path.c_str();
            item.length     = 0;
            m_playlist.InsertItem(item);
        }
    }
}

void CMainWindow::OnPlaylistRemoveClick(unsigned int itemIdx)
{
    m_playlist.DeleteItem(itemIdx);
}

void CMainWindow::OnPlaylistSaveClick()
{
    Win32::CFileDialog dialog;
    const TCHAR* filter = PLAYLIST_FILTER;
    dialog.m_OFN.lpstrFilter = filter;
    dialog.m_OFN.lpstrDefExt = PLAYLIST_EXTENSION;
    if(dialog.SummonSave(m_hWnd))
    {
        m_playlist.Write(string_cast<string>(dialog.GetPath()).c_str());
    }
}

void CMainWindow::OnTrayIconEvent(Win32::CTrayIcon* icon, LPARAM param)
{
    switch(param)
    {
    case WM_LBUTTONUP:
        Show(SW_SHOWNORMAL);
        SetForegroundWindow(m_hWnd);
        break;
    case WM_RBUTTONUP:
        Show(SW_SHOWNORMAL);
        SetForegroundWindow(m_hWnd);
        DisplayTrayMenu();
        break;
    }
}

void CMainWindow::DisplayTrayMenu()
{
	POINT p;
	SetForegroundWindow(m_trayIconServer.m_hWnd);
	GetCursorPos(&p);
	TrackPopupMenu(GetSubMenu(m_popupMenu, 0), 0, p.x, p.y, NULL, m_hWnd, NULL);
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
				else
				{
					Reset();
					UpdateButtons();
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
    const int fps = 60;
    int time = m_frames / fps;
    int seconds = time % 60;
    int minutes = time / 60;
    tstring timerText = lexical_cast_uint<tstring>(minutes, 2) + _T(":") + lexical_cast_uint<tstring>(seconds, 2);
    m_timerLabel->SetText(timerText.c_str());
}

void CMainWindow::UpdateTitle()
{
    tstring titleLabelText = APP_NAME;
    tstring windowText = APP_NAME;

    if(m_tags.HasTag("title"))
    {
        wstring titleTag = m_tags.GetTagValue("title");

        titleLabelText = string_cast<tstring>(titleTag);

	    windowText += _T(" - [ ");
	    windowText += string_cast<tstring>(titleTag);
	    windowText += _T(" ]");
    }

    m_titleLabel->SetText(titleLabelText.c_str());
    SetText(windowText.c_str());
}

void CMainWindow::UpdateButtons()
{
    CVirtualMachine::STATUS status = m_virtualMachine.GetStatus();
    if(m_pauseButton != NULL)
    {
        m_pauseButton->Enable(m_ready ? TRUE : FALSE);
        m_pauseButton->SetText((status == CVirtualMachine::PAUSED) ? _T("Play") : _T("Pause"));
    }
}

void CMainWindow::UpdateRepeatButton()
{
	switch(m_repeatMode)
	{
	case PLAYLIST_ONCE:
		m_repeatButton->SetText(_T("PL Once"));
		break;
	case PLAYLIST_REPEAT:
		m_repeatButton->SetText(_T("PL Repeat"));
		break;
	case TRACK_REPEAT:
		m_repeatButton->SetText(_T("Track Repeat"));
		break;
	}
}

void CMainWindow::CreateAudioPluginMenu()
{
    Win32::CMenuItem pluginSubMenu(CreatePopupMenu());

    for(unsigned int i = 0; m_handlerInfo[i].name != NULL; i++)
    {
        tstring caption = m_handlerInfo[i].name;
        InsertMenu(pluginSubMenu, i, MF_STRING, ID_FILE_AUDIOPLUGIN_PLUGIN_0 + i, caption.c_str());
    }

    Win32::CMenuItem pluginMenu(Win32::CMenuItem::FindById(m_popupMenu, ID_FILE_AUDIOPLUGIN));
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
        Win32::CMenuItem pluginSubMenuEntry(Win32::CMenuItem::FindById(m_popupMenu, ID_FILE_AUDIOPLUGIN_PLUGIN_0 + i));
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
    Win32::CMenuItem pluginSubMenu(CreatePopupMenu());

    for(unsigned int i = 0; m_charEncodingInfo[i].name != NULL; i++)
    {
        tstring caption = m_charEncodingInfo[i].name;
        InsertMenu(pluginSubMenu, i, MF_STRING, ID_FILE_CHARENCODING_ENCODING_0 + i, caption.c_str());
    }

    Win32::CMenuItem pluginMenu(Win32::CMenuItem::FindById(m_popupMenu, ID_FILE_CHARACTERENCODING));
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
        Win32::CMenuItem pluginSubMenuEntry(Win32::CMenuItem::FindById(m_popupMenu, ID_FILE_CHARENCODING_ENCODING_0 + i));
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
	Win32::CAcceleratorTableGenerator tableGenerator;
	tableGenerator.Insert(ID_FILE_PAUSE,			VK_F5,	FVIRTKEY);
//	tableGenerator.Insert(ID_SETTINGS_ENABLEREVERB,	'R',	FVIRTKEY | FCONTROL);	
	return tableGenerator.Create();
}

void CMainWindow::OnNewFrame()
{
	m_frames++;
}

void CMainWindow::OnFileOpen()
{
    Win32::CFileDialog dialog;
    const TCHAR* filter = 
	    _T("All Supported Files\0*.psfpl; *.psf; *.minipsf; *.psf2; *.minipsf2; *.psfp; *.minipsfp;") PLAYLIST_EXTENSION _T("\0")
        PLAYLIST_FILTER
	    PSF_FILTER
	    PSF2_FILTER
		PSFP_FILTER;
    dialog.m_OFN.lpstrFilter = filter;
    if(dialog.SummonOpen(m_hWnd))
    {
        tstring file = dialog.GetPath();
        tstring extension;
        tstring::size_type dotPosition = file.find('.');
        if(dotPosition != tstring::npos)
        {
            extension = tstring(file.begin() + dotPosition + 1, file.end());
        }
        if(!wcsicmp(extension.c_str(), PLAYLIST_EXTENSION))
        {
            LoadPlaylist(string_cast<string>(file).c_str());
        }
        else
        {
	        LoadSingleFile(string_cast<string>(file).c_str());
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
    UpdateButtons();
}

void CMainWindow::OnPrev()
{
    if(m_playlist.GetItemCount() == 0) return;
    if(m_currentPlaylistItem != 0)
    {
        m_currentPlaylistItem--;        
    }
    OnPlaylistItemDblClick(m_currentPlaylistItem);
}

void CMainWindow::OnNext()
{
    if(m_playlist.GetItemCount() == 0) return;
    unsigned int itemCount = m_playlist.GetItemCount();
    if((m_currentPlaylistItem + 1) < itemCount)
    {
        m_currentPlaylistItem++;
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
	case PLAYLIST_ONCE:
		m_repeatMode = PLAYLIST_REPEAT;
		break;
	case PLAYLIST_REPEAT:
		m_repeatMode = TRACK_REPEAT;
		break;
	case TRACK_REPEAT:
		m_repeatMode = PLAYLIST_ONCE;
		break;
	}
	UpdateRepeatButton();
}

void CMainWindow::ActivatePanel(unsigned int panelIdx)
{
	if(m_currentPanel != -1)
	{
		m_panels[m_currentPanel]->Show(SW_HIDE);
	}
	m_currentPanel = panelIdx;
	m_panels[m_currentPanel]->RefreshLayout();
	m_panels[m_currentPanel]->Show(SW_SHOW);
}

void CMainWindow::OnAbout()
{
    CAboutWindow about(m_hWnd);
    about.DoModal();
}

void CMainWindow::LoadSingleFile(const char* path)
{
    if(PlayFile(path))
    {
        m_playlist.Clear();
        m_currentPlaylistItem = 0;

        CPlaylist::ITEM item;
        item.path = path;
        CPlaylist::PopulateItemFromTags(item, m_tags);
        m_playlist.InsertItem(item);
    }
}

void CMainWindow::LoadPlaylist(const char* path)
{
    m_playlist.Clear();
    m_playlist.Read(path);
    if(m_playlist.GetItemCount() > 0)
    {
        m_currentPlaylistItem = 0;
        OnPlaylistItemDblClick(m_currentPlaylistItem);
    }
}

void CMainWindow::Reset()
{
	m_spu0RegViewPanel->SetSpu(NULL);
	m_spu1RegViewPanel->SetSpu(NULL);
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();
    m_frames = 0;
	m_trackLength = 0;
	m_fadePosition = 0;
	m_ready = false;
	m_volumeAdjust = 1.0f;
}

bool CMainWindow::PlayFile(const char* path)
{
	Reset();
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(m_virtualMachine, path, &tags);
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
		m_virtualMachine.Resume();
		m_ready = true;
		if(m_repeatMode == PLAYLIST_REPEAT || m_repeatMode == PLAYLIST_ONCE)
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
	catch(const exception& except)
	{
		tstring errorString = _T("Couldn't load PSF file: \r\n\r\n");
		errorString += string_cast<tstring>(except.what());
		MessageBox(m_hWnd, errorString.c_str(), NULL, 16);
	}

	UpdateTitle();
    UpdateButtons();
    UpdateClock();
//	UpdateMenu();

    return m_ready;
}
