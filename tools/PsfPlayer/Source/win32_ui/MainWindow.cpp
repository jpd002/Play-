#include "AppDef.h"
#include "AppConfig.h"
#include "MainWindow.h"
#include "AboutWindow.h"
#include "../PsfLoader.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "win32/MenuItem.h"
#include "layout/LayoutEngine.h"
#include "placeholder_def.h"
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "resource.h"
#include <afxres.h>

#define CLSNAME			                _T("MainWindow")
#define WNDSTYLE		                (WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX)
#define WNDSTYLEEX		                (0)
#define WM_UPDATEVIS	                (WM_USER + 1)

#define ID_FILE_AUDIOPLUGIN_PLUGIN_0    (0xBEEF)

CMainWindow::SPUHANDLER_INFO CMainWindow::m_handlerInfo[] =
{
    {   1,      _T("Win32 WaveOut"),    _T("SH_WaveOut.dll")   },
    {   2,      _T("OpenAL"),           _T("SH_OpenAL.dll")    },
    {   NULL,   NULL,                   NULL                   },
};

using namespace Framework;
using namespace std;

CMainWindow::CMainWindow(CPsfVm& virtualMachine) :
m_virtualMachine(virtualMachine),
m_ready(false),
m_frames(0),
m_writes(0),
m_ejectButton(NULL),
m_pauseButton(NULL),
m_aboutButton(NULL)
{
    m_selectedAudioHandler = m_handlerInfo[0].id;

	if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&Win32::CWindow::MakeWndClass(CLSNAME));
	}

//	Create(WNDSTYLEEX, CLSNAME, APP_NAME, WNDSTYLE, Win32::CRect(0, 0, 320, 480), NULL, NULL);
	Create(WNDSTYLEEX, CLSNAME, APP_NAME, WNDSTYLE, Win32::CRect(0, 0, 200, 175), NULL, NULL);
    SetClassPtr();

	SetTimer(m_hWnd, 0, 1000, NULL);

	//UpdateTitle();
	//UpdateFromConfig();

	SetIcon(ICON_SMALL, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN)));
    m_popupMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_POPUP));

    m_virtualMachine.OnNewFrame.connect(std::tr1::bind(&CMainWindow::OnNewFrame, this));
    m_virtualMachine.OnBufferWrite.connect(std::tr1::bind(&CMainWindow::OnBufferWrite, this, std::tr1::placeholders::_1));

	m_virtualMachine.SetSpuHandler(std::tr1::bind(&CMainWindow::CreateHandler, m_handlerInfo[0].dllName));

    m_timerLabel = new Win32::CStatic(m_hWnd, _T(""), SS_CENTER);
    m_titleLabel = new Win32::CStatic(m_hWnd, _T(""), SS_CENTER | SS_NOPREFIX);

    m_nextButton = new Win32::CButton(_T(">>"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_prevButton = new Win32::CButton(_T("<<"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_pauseButton = new Win32::CButton(_T("Pause"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_ejectButton = new Win32::CButton(_T("Eject"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    
    m_aboutButton = new Win32::CButton(_T("About"), m_hWnd, Win32::CRect(0, 0, 1, 1));

    m_layout = 
        VerticalLayoutContainer(
            LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_titleLabel)) +
            LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_timerLabel)) +
            LayoutExpression(CLayoutStretch::Create()) +
            HorizontalLayoutContainer(
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_prevButton)) +
                LayoutExpression(CLayoutStretch::Create()) +
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_pauseButton)) +
                LayoutExpression(CLayoutStretch::Create()) +
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_nextButton))
            ) +
            HorizontalLayoutContainer(
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_ejectButton)) +
                LayoutExpression(CLayoutStretch::Create()) +
                LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 30, m_aboutButton))
            )
        );

    Win32::CTrayIcon* trayIcon = m_trayIconServer.Insert();
    trayIcon->SetIcon(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN)));
    trayIcon->SetTip(_T("PsfPlayer"));

    m_trayIconServer.RegisterHandler(std::tr1::bind(&CMainWindow::OnTrayIconEvent, this, 
        std::tr1::placeholders::_1, std::tr1::placeholders::_2));

    CreateAudioPluginMenu();
    UpdateAudioPluginMenu();
    UpdateTimer();
    UpdateTitle();
    UpdateButtons();
	RefreshLayout();
}

CMainWindow::~CMainWindow()
{
	m_virtualMachine.Pause();
}

void CMainWindow::RefreshLayout()
{
	RECT rc = GetClientRect();
	m_layout->SetRect(rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);
	m_layout->RefreshGeometry();
}

CSoundHandler* CMainWindow::CreateHandler(const TCHAR* libraryPath)
{
	typedef CSoundHandler* (*HandlerFactoryFunction)();
	HMODULE module = LoadLibrary(libraryPath);
	HandlerFactoryFunction handlerFactory = reinterpret_cast<HandlerFactoryFunction>(GetProcAddress(module, "HandlerFactory"));
	CSoundHandler* result = handlerFactory();
	return result;
}

long CMainWindow::OnCommand(unsigned short nID, unsigned short nCmd, HWND hControl)
{
    if(m_ejectButton && m_ejectButton->m_hWnd == hControl)
    {
        OnFileOpen();
    }
    else if(m_pauseButton && m_pauseButton->m_hWnd == hControl)
    {
        OnPause();
    }
    else if(m_aboutButton && m_aboutButton->m_hWnd == hControl)
    {
        OnAbout();
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
	    }
    }
	return TRUE;    
}

long CMainWindow::OnTimer()
{
    UpdateTimer();
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

void CMainWindow::UpdateTimer()
{
    const int fps = 60;
    int time = m_frames / fps;
    int seconds = time % 60;
    int minutes = time / 60;
    tstring timerText = lexical_cast_uint<tstring>(minutes, 2) + _T(":") + lexical_cast_uint<tstring>(seconds, 2);
    m_timerLabel->SetText(timerText.c_str());

    //tstring timerText = lexical_cast_uint<tstring>(m_writes);
    //m_timerLabel->SetText(timerText.c_str());
    //m_writes = 0;
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
        pluginSubMenuEntry.Check(m_handlerInfo[i].id == m_selectedAudioHandler);
    }
}

void CMainWindow::ChangeAudioPlugin(unsigned int pluginIdx)
{
    SPUHANDLER_INFO* handlerInfo = m_handlerInfo + pluginIdx;
    m_selectedAudioHandler = handlerInfo->id;
	m_virtualMachine.SetSpuHandler(std::tr1::bind(&CMainWindow::CreateHandler, handlerInfo->dllName));
    UpdateAudioPluginMenu();
}

void CMainWindow::OnNewFrame()
{
	m_frames++;
}

void CMainWindow::OnBufferWrite(int writeSize)
{
    m_writes += writeSize;
}

void CMainWindow::OnFileOpen()
{
    Win32::CFileDialog dialog;
    const TCHAR* filter = 
	    _T("All Supported Files\0*.psf; *.minipsf; *.psf2; *.minipsf2\0")
	    _T("PlayStation Sound Files (*.psf; *.minipsf)\0*.psf; *.minipsf\0")
	    _T("PlayStation2 Sound Files (*.psf2; *.minipsf2)\0*.psf2; *.minipsf2\0");
    dialog.m_OFN.lpstrFilter = filter;
    if(dialog.Summon(m_hWnd))
    {
	    Load(string_cast<string>(dialog.m_sFile).c_str());
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

void CMainWindow::OnAbout()
{
    CAboutWindow about(m_hWnd);
    about.DoModal();
}

void CMainWindow::Load(const char* path)
{
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();
    m_frames = 0;
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(m_virtualMachine, path, &tags);
		m_tags = CPsfTags(tags);
		try
		{
			float volumeAdjust = boost::lexical_cast<float>(m_tags.GetTagValue("volume"));
			m_virtualMachine.SetVolumeAdjust(volumeAdjust);
		}
		catch(...)
		{

		}
		m_virtualMachine.Resume();
		m_ready = true;
	}
	catch(const exception& except)
	{
		tstring errorString = _T("Couldn't load PSF file: \r\n\r\n");
		errorString += string_cast<tstring>(except.what());
		MessageBox(m_hWnd, errorString.c_str(), NULL, 16);
		m_ready = false;
	}
	UpdateTitle();
    UpdateButtons();
    UpdateTimer();
//	UpdateMenu();
}
