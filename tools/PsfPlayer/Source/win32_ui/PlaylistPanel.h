#ifndef _PLAYLISTPANEL_H_
#define _PLAYLISTPANEL_H_

#include "Playlist.h"
#include "win32/Dialog.h"
#include "win32/Layouts.h"
#include "win32/ListView.h"
#include "win32/Button.h"
#include "win32/GdiObj.h"
#include <boost/signals2.hpp>

class CPlaylistPanel : public Framework::Win32::CDialog, public boost::signals2::trackable
{
public:
	typedef boost::signals2::signal<void (unsigned int)>	OnItemDblClickEvent;
	typedef boost::signals2::signal<void ()>				OnAddClickEvent;
	typedef boost::signals2::signal<void (unsigned int)>	OnRemoveClickEvent;
	typedef boost::signals2::signal<void ()>				OnSaveClickEvent;

										CPlaylistPanel(HWND, CPlaylist&);
	virtual								~CPlaylistPanel();

	void								SetPlayingItemIndex(unsigned int);
	void								RefreshLayout();

	OnItemDblClickEvent					OnItemDblClick;
	OnAddClickEvent						OnAddClick;
	OnRemoveClickEvent					OnRemoveClick;
	OnSaveClickEvent					OnSaveClick;

protected:
	long								OnCommand(unsigned short, unsigned short, HWND) override;
	LRESULT								OnNotify(WPARAM, NMHDR*) override;
	long								OnSize(unsigned int, unsigned int, unsigned int) override;

private:
	void								CreateColumns();
	void								OnPlaylistItemInsert(const CPlaylist::ITEM&);
	void								OnPlaylistItemUpdate(unsigned int, const CPlaylist::ITEM&);
	void								OnPlaylistItemDelete(unsigned int);
	void								OnPlaylistItemsClear();
	void								AddItem(const TCHAR*, const TCHAR*);
	void								ModifyItem(int, const TCHAR*, const TCHAR*);
	void								ExchangeItems(unsigned int, unsigned int);
	void								OnMoveUpButtonClick();
	void								OnMoveDownButtonClick();
	void								OnAddButtonClick();
	void								OnRemoveButtonClick();
	void								OnSaveButtonClick();
	void								OnPlaylistViewDblClick(NMITEMACTIVATE*);
	void								OnPlaylistViewCustomDraw(NMLVCUSTOMDRAW*);

	Framework::LayoutObjectPtr			m_layout;

	int									m_playingItemIndex;
	Framework::Win32::CFont				m_playingItemFont;

	CPlaylist&							m_playlist;
	Framework::Win32::CListView*		m_playlistView;
	Framework::Win32::CButton*			m_moveUpButton;
	Framework::Win32::CButton*			m_moveDownButton;
	Framework::Win32::CButton*			m_addButton;
	Framework::Win32::CButton*			m_removeButton;
	Framework::Win32::CButton*			m_saveButton;
};

#endif
