#ifndef _PLAYLISTPANEL_H_
#define _PLAYLISTPANEL_H_

#include "Playlist.h"
#include "win32/Dialog.h"
#include "win32/Layouts.h"
#include "win32/ListView.h"
#include "win32/Button.h"
#include "win32/GdiObj.h"
#include "signal/Signal.h"

class CPlaylistPanel : public Framework::Win32::CDialog
{
public:
	typedef Framework::CSignal<void(unsigned int)> OnItemDblClickEvent;
	typedef Framework::CSignal<void()> OnAddClickEvent;
	typedef Framework::CSignal<void(unsigned int)> OnRemoveClickEvent;
	typedef Framework::CSignal<void()> OnSaveClickEvent;

	CPlaylistPanel(HWND, CPlaylist&);
	virtual ~CPlaylistPanel();

	void SetPlayingItemIndex(unsigned int);
	void RefreshLayout();

	OnItemDblClickEvent OnItemDblClick;
	OnAddClickEvent OnAddClick;
	OnRemoveClickEvent OnRemoveClick;
	OnSaveClickEvent OnSaveClick;

protected:
	long OnCommand(unsigned short, unsigned short, HWND) override;
	LRESULT OnNotify(WPARAM, NMHDR*) override;
	long OnSize(unsigned int, unsigned int, unsigned int) override;

private:
	void CreateColumns();
	void OnPlaylistItemInsert(const CPlaylist::ITEM&);
	void OnPlaylistItemUpdate(unsigned int, const CPlaylist::ITEM&);
	void OnPlaylistItemDelete(unsigned int);
	void OnPlaylistItemsClear();
	void AddItem(const TCHAR*, const TCHAR*);
	void ModifyItem(int, const TCHAR*, const TCHAR*);
	void ExchangeItems(unsigned int, unsigned int);
	void OnMoveUpButtonClick();
	void OnMoveDownButtonClick();
	void OnAddButtonClick();
	void OnRemoveButtonClick();
	void OnSaveButtonClick();
	void OnPlaylistViewDblClick(NMITEMACTIVATE*);
	void OnPlaylistViewCustomDraw(NMLVCUSTOMDRAW*);

	Framework::LayoutObjectPtr m_layout;

	int m_playingItemIndex;
	Framework::Win32::CFont m_playingItemFont;

	CPlaylist& m_playlist;
	Framework::Win32::CListView* m_playlistView;
	Framework::Win32::CButton* m_moveUpButton;
	Framework::Win32::CButton* m_moveDownButton;
	Framework::Win32::CButton* m_addButton;
	Framework::Win32::CButton* m_removeButton;
	Framework::Win32::CButton* m_saveButton;

	CPlaylist::OnItemInsertEvent::Connection m_OnItemInsertConnection;
	CPlaylist::OnItemUpdateEvent::Connection m_OnItemUpdateConnection;
	CPlaylist::OnItemDeleteEvent::Connection m_OnItemDeleteConnection;
	CPlaylist::OnItemsClearEvent::Connection m_OnItemsClearConnection;
};

#endif
