#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <deque>
#include <thread>
#include <boost/filesystem/path.hpp>
#include "../PsfVm.h"
#include "../PsfBase.h"
#include "../PsfTags.h"
#include "../Playlist.h"
#include "win32/Dialog.h"
#include "win32/Static.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include "win32/TrayIconServer.h"
#include "win32/ToolTip.h"
#include "win32/TaskBarList.h"
#include "win32/GdiObj.h"
#include "PlaylistPanel.h"
#include "FileInformationPanel.h"
#include "SpuRegViewPanel.h"
#include "AcceleratorTable.h"
#include "LockFreeQueue.h"

class CMainWindow : public Framework::Win32::CDialog, public boost::signals2::trackable
{
public:
										CMainWindow(CPsfVm&);
	virtual								~CMainWindow();

	void								Run();

protected:
	long								OnWndProc(unsigned int, WPARAM, LPARAM);
	long								OnCommand(unsigned short, unsigned short, HWND);
	long								OnSize(unsigned int, unsigned int, unsigned int);
	long								OnTimer(WPARAM);
	long								OnClose();

private:
	enum
	{
		MAX_PANELS = 4,
	};

	enum
	{
		TIMER_UPDATE_CLOCK,
		TIMER_UPDATE_FADE,
		TIMER_UPDATE_DISCOVERIES,
	};

	enum REPEAT_MODE
	{
		PLAYLIST_ONCE,
		PLAYLIST_REPEAT,
		PLAYLIST_SHUFFLE,
		TRACK_REPEAT
	};

	struct SOUNDHANDLER_INFO
	{
		int				id;
		const TCHAR*	name;
		const TCHAR*	dllName;
	};

	struct CHARENCODING_INFO
	{
		int				id;
		const TCHAR*	name;
	};

	struct DISCOVERY_COMMAND
	{
		boost::filesystem::path		filePath;
		boost::filesystem::path		archivePath;
		unsigned int				runId;
		unsigned int				itemId;
	};

	struct DISCOVERY_RESULT
	{
		unsigned int	runId;
		unsigned int	itemId;
		std::wstring	title;
		double			length;
	};

	typedef std::deque<DISCOVERY_COMMAND> DiscoveryCommandQueue;
	typedef std::deque<DISCOVERY_RESULT> DiscoveryResultQueue;

	CSoundHandler*						CreateHandler(const TCHAR*);

	void								OnNewFrame();

	void								OnFileOpen();
	void								OnPause();
	void								OnPrev();
	void								OnNext();
	void								OnPrevPanel();
	void								OnNextPanel();
	void								OnAbout();
	void								OnRepeat();
	void								OnConfig();

	static LRESULT CALLBACK				MessageHookProc(int, WPARAM, LPARAM);

	bool								PlayFile(const boost::filesystem::path&, const boost::filesystem::path&);
	void								LoadSingleFile(const boost::filesystem::path&);
	void								LoadPlaylist(const boost::filesystem::path&);
	void								LoadArchive(const boost::filesystem::path&);

	void								OnPlaylistItemDblClick(unsigned int);
	void								OnPlaylistAddClick();
	void								OnPlaylistRemoveClick(unsigned int);
	void								OnPlaylistSaveClick();

	void								OnClickReverbEnabled();

	void								OnTrayIconEvent(Framework::Win32::CTrayIcon*, LPARAM);
	void								DisplayTrayMenu();
	void								UpdateConfigMenu();
	void								UpdateClock();
	void								UpdateFade();
	void								UpdateTitle();
	void								UpdatePlaybackButtons();
	void								UpdateRepeatButton();

	void								Reset();
	void								ActivatePanel(unsigned int);

	void								CreateAudioPluginMenu();
	void								UpdateAudioPluginMenu();
	void								ChangeAudioPlugin(unsigned int);
	void								LoadAudioPluginPreferences();
	int									FindAudioPlugin(unsigned int);

	void								CreateCharEncodingMenu();
	void								UpdateCharEncodingMenu();
	void								ChangeCharEncoding(unsigned int);
	void								LoadCharEncodingPreferences();
	int									FindCharEncoding(unsigned int);

	void								ResetDiscoveryRun();
	void								AddDiscoveryItem(const boost::filesystem::path&, const boost::filesystem::path&, unsigned int);
	void								ProcessPendingDiscoveries();
	void								DiscoveryThreadProc();

	HACCEL								CreateAccelerators();
	void								CreateSymbolFonts();

	static uint32						GetNextRandomNumber(uint32);
	static uint32						GetPrevRandomNumber(uint32);

	Framework::Win32::CStatic*			m_timerLabel;
	Framework::Win32::CStatic*			m_titleLabel;

	Framework::Win32::CButton*			m_repeatButton;
	Framework::Win32::CButton*			m_configButton;

	Framework::Win32::CStatic*			m_placeHolder;

	Framework::Win32::CButton*			m_pauseButton;

	Framework::Win32::CToolTip*			m_toolTip;

	bool								m_useTrayIcon;

	Framework::Win32::CTrayIconServer*	m_trayIconServer;
	Framework::Win32::CTaskBarList*		m_taskBarList;

	Framework::Win32::CWindow*			m_panels[MAX_PANELS];
	CPlaylistPanel*						m_playlistPanel;
	CFileInformationPanel*				m_fileInformationPanel;
	CSpuRegViewPanel*					m_spu0RegViewPanel;
	CSpuRegViewPanel*					m_spu1RegViewPanel;

	HMENU								m_trayPopupMenu;
	HMENU								m_configPopupMenu;

	static HHOOK						g_messageFilterHook;
	static HWND							g_messageFilterHookWindow;

	Framework::Win32::CAcceleratorTable	m_accel;

	HICON								m_playListOnceIcon;
	HICON								m_repeatListIcon;
	HICON								m_shuffleListIcon;
	HICON								m_repeatTrackIcon;
	HICON								m_configIcon;
	HICON								m_playIcon;
	HICON								m_pauseIcon;
	HICON								m_prevTrackIcon;
	HICON								m_nextTrackIcon;

	CPsfVm&								m_virtualMachine;
	CPsfTags							m_tags;
	CPlaylist							m_playlist;
	unsigned int						m_currentPlaylistItem;
	unsigned int						m_currentPanel;
	bool								m_ready;
	uint64								m_frames;
	uint64								m_lastUpdateTime;
	uint64								m_trackLength;
	uint64								m_fadePosition;
	float								m_volumeAdjust;
	int									m_selectedAudioPlugin;
	int									m_selectedCharEncoding;
	REPEAT_MODE							m_repeatMode;
	bool								m_reverbEnabled;
	uint32								m_randomSeed;
	Framework::Win32::CFont				m_webdingsFont;
	Framework::Win32::CFont				m_segoeUiSymbolFont;

	std::thread							m_discoveryThread;
	bool								m_discoveryThreadActive;
	CLockFreeQueue<DISCOVERY_COMMAND>	m_discoveryCommandQueue;
	CLockFreeQueue<DISCOVERY_RESULT>	m_discoveryResultQueue;
	DiscoveryCommandQueue				m_pendingDiscoveryCommands;
	uint32								m_discoveryRunId;

	static SOUNDHANDLER_INFO			m_handlerInfo[];
	static CHARENCODING_INFO			m_charEncodingInfo[];
};

#endif
