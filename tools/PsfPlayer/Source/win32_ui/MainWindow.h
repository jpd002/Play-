#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "../PsfVm.h"
#include "../PsfBase.h"
#include "../PsfTags.h"
#include "win32/Window.h"
#include "win32/Static.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include "win32/TrayIconServer.h"

class CMainWindow : public Framework::Win32::CWindow
{
public:
				                        CMainWindow(CPsfVm&);
	virtual		                        ~CMainWindow();

    void                                RefreshLayout();

protected:
    long                                OnCommand(unsigned short, unsigned short, HWND);
    long                                OnSize(unsigned int, unsigned int, unsigned int);
    long                                OnTimer();

private:
    struct SPUHANDLER_INFO
    {
        int             id;
        const TCHAR*    name;
        const TCHAR*    dllName;
    };

	static CSoundHandler*				CreateHandler(const TCHAR*);

    void                                OnNewFrame();
    void                                OnBufferWrite(int);

    void                                OnFileOpen();
    void                                OnPause();
    void                                OnAbout();
    void                                Load(const char*);

    void                                OnTrayIconEvent(Framework::Win32::CTrayIcon*, LPARAM);
    void                                DisplayTrayMenu();
    void                                UpdateTimer();
    void                                UpdateTitle();
    void                                UpdateButtons();
    void                                CreateAudioPluginMenu();
    void                                UpdateAudioPluginMenu();

    void                                ChangeAudioPlugin(unsigned int);

    Framework::Win32::CStatic*          m_timerLabel;
    Framework::Win32::CStatic*          m_titleLabel;
	Framework::LayoutObjectPtr	        m_layout;

    Framework::Win32::CButton*          m_nextButton;
    Framework::Win32::CButton*          m_prevButton;
    Framework::Win32::CButton*          m_pauseButton;
    Framework::Win32::CButton*          m_ejectButton;
    Framework::Win32::CButton*          m_aboutButton;

    Framework::Win32::CTrayIconServer   m_trayIconServer;

    HMENU                               m_popupMenu;

	CPsfVm&								m_virtualMachine;
	CPsfTags							m_tags;
	bool								m_ready;
    uint64                              m_frames;
    int                                 m_writes;
    int                                 m_selectedAudioHandler;

    static SPUHANDLER_INFO              m_handlerInfo[];
};

#endif
