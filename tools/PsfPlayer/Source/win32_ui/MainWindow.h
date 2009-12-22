#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "../PsfVm.h"
#include "../PsfBase.h"
#include "../PsfTags.h"
#include "../Playlist.h"
#include "win32/Window.h"
#include "win32/Static.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include "win32/TrayIconServer.h"
#include "Panel.h"
#include "PlaylistPanel.h"
#include "FileInformationPanel.h"
#include "AcceleratorTable.h"

class CMainWindow : public Framework::Win32::CWindow, public boost::signals::trackable
{
public:
				                        CMainWindow(CPsfVm&);
	virtual		                        ~CMainWindow();

	void								Run();
    void                                RefreshLayout();

protected:
    long                                OnCommand(unsigned short, unsigned short, HWND);
    long                                OnSize(unsigned int, unsigned int, unsigned int);
    long                                OnTimer();

private:
    enum
    {
        MAX_PANELS = 2,
    };

    struct SPUHANDLER_INFO
    {
        int             id;
        const TCHAR*    name;
        const TCHAR*    dllName;
    };

	CSoundHandler*				        CreateHandler(const TCHAR*);

    void                                OnNewFrame();
    void                                OnBufferWrite(int);

    void                                OnFileOpen();
    void                                OnPause();
    void                                OnPrev();
    void                                OnNext();
	void								OnPrevPanel();
	void								OnNextPanel();
    void                                OnAbout();
    bool                                PlayFile(const char*);
    void                                LoadSingleFile(const char*);
    void                                LoadPlaylist(const char*);

    void                                OnPlaylistItemDblClick(unsigned int);
    void                                OnPlaylistAddClick();
    void                                OnPlaylistRemoveClick(unsigned int);
    void                                OnPlaylistSaveClick();

    void                                OnTrayIconEvent(Framework::Win32::CTrayIcon*, LPARAM);
    void                                DisplayTrayMenu();
    void                                UpdateTimer();
    void                                UpdateTitle();
    void                                UpdateButtons();
    void                                CreateAudioPluginMenu();
    void                                UpdateAudioPluginMenu();

	void								ActivatePanel(unsigned int);

	void                                ChangeAudioPlugin(unsigned int);

	HACCEL								CreateAccelerators();

    Framework::Win32::CStatic*          m_timerLabel;
    Framework::Win32::CStatic*          m_titleLabel;
	Framework::LayoutObjectPtr	        m_layout;

	Framework::Win32::CButton*			m_nextPanelButton;
	Framework::Win32::CButton*			m_prevPanelButton;
	Framework::Win32::CButton*			m_repeatButton;

	Framework::Win32::CStatic*          m_placeHolder;

    Framework::Win32::CButton*          m_nextButton;
    Framework::Win32::CButton*          m_prevButton;
    Framework::Win32::CButton*          m_pauseButton;
    Framework::Win32::CButton*          m_ejectButton;

	Framework::Win32::CTrayIconServer   m_trayIconServer;

    CPanel*                             m_panels[MAX_PANELS];
    CPlaylistPanel*                     m_playlistPanel;
	CFileInformationPanel*				m_fileInformationPanel;

    HMENU                               m_popupMenu;
	Framework::Win32::CAcceleratorTable	m_accel;

	CPsfVm&								m_virtualMachine;
	CPsfTags							m_tags;
    CPlaylist                           m_playlist;
    unsigned int                        m_currentPlaylistItem;
	unsigned int						m_currentPanel;
    bool								m_ready;
    uint64                              m_frames;
    int                                 m_writes;
    int                                 m_selectedAudioHandler;

    static SPUHANDLER_INFO              m_handlerInfo[];
};

#endif
