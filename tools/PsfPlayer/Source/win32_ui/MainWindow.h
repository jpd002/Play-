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
#include "SpuRegViewPanel.h"
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
    long                                OnTimer(WPARAM);

private:
    enum
    {
        MAX_PANELS = 4,
    };

	enum
	{
		TIMER_UPDATE_CLOCK,
		TIMER_UPDATE_FADE,
	};

	enum REPEAT_MODE
	{
		PLAYLIST_ONCE,
		PLAYLIST_REPEAT,
		TRACK_REPEAT,
	};

    struct SOUNDHANDLER_INFO
    {
        int             id;
        const TCHAR*    name;
        const TCHAR*    dllName;
    };

	struct CHARENCODING_INFO
	{
		int				id;
		const TCHAR*	name;
	};

	CSoundHandler*				        CreateHandler(const TCHAR*);

    void                                OnNewFrame();

    void                                OnFileOpen();
    void                                OnPause();
    void                                OnPrev();
    void                                OnNext();
	void								OnPrevPanel();
	void								OnNextPanel();
    void                                OnAbout();
	void								OnRepeat();
    bool                                PlayFile(const char*);
    void                                LoadSingleFile(const char*);
    void                                LoadPlaylist(const char*);

    void                                OnPlaylistItemDblClick(unsigned int);
    void                                OnPlaylistAddClick();
    void                                OnPlaylistRemoveClick(unsigned int);
    void                                OnPlaylistSaveClick();

    void                                OnTrayIconEvent(Framework::Win32::CTrayIcon*, LPARAM);
    void                                DisplayTrayMenu();
    void                                UpdateClock();
	void								UpdateFade();
    void                                UpdateTitle();
    void                                UpdateButtons();
	void								UpdateRepeatButton();

	void								Reset();
	void								ActivatePanel(unsigned int);

    void                                CreateAudioPluginMenu();
    void                                UpdateAudioPluginMenu();
	void                                ChangeAudioPlugin(unsigned int);
	void								LoadAudioPluginPreferences();
	int									FindAudioPlugin(unsigned int);

	void								CreateCharEncodingMenu();
	void								UpdateCharEncodingMenu();
	void								ChangeCharEncoding(unsigned int);
	void								LoadCharEncodingPreferences();
	int									FindCharEncoding(unsigned int);

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
	CSpuRegViewPanel*					m_spu0RegViewPanel;
	CSpuRegViewPanel*					m_spu1RegViewPanel;

    HMENU                               m_popupMenu;
	Framework::Win32::CAcceleratorTable	m_accel;

	CPsfVm&								m_virtualMachine;
	CPsfTags							m_tags;
    CPlaylist                           m_playlist;
    unsigned int                        m_currentPlaylistItem;
	unsigned int						m_currentPanel;
    bool								m_ready;
    uint64                              m_frames;
	uint64								m_trackLength;
	uint64								m_fadePosition;
	float								m_volumeAdjust;
    int                                 m_selectedAudioPlugin;
	int									m_selectedCharEncoding;
	REPEAT_MODE							m_repeatMode;

    static SOUNDHANDLER_INFO			m_handlerInfo[];
	static CHARENCODING_INFO			m_charEncodingInfo[];
};

#endif
