#ifndef _PLAYLISTPANEL_H_
#define _PLAYLISTPANEL_H_

#include "Playlist.h"
#include "win32/Dialog.h"
#include "win32/Layouts.h"
#include "win32/ListView.h"
#include "win32/Button.h"

class CPlaylistPanel : public Framework::Win32::CDialog, public boost::signals::trackable
{
public:
    typedef boost::signal<void (unsigned int)>  OnItemDblClickEvent;
    typedef boost::signal<void ()>              OnAddClickEvent;
    typedef boost::signal<void (unsigned int)>  OnRemoveClickEvent;
    typedef boost::signal<void ()>              OnSaveClickEvent;

                                        CPlaylistPanel(HWND, CPlaylist&);
    virtual                             ~CPlaylistPanel();

    void                                RefreshLayout();

    OnItemDblClickEvent                 OnItemDblClick;
    OnAddClickEvent                     OnAddClick;
    OnRemoveClickEvent                  OnRemoveClick;
    OnSaveClickEvent                    OnSaveClick;

protected:
    long                                OnCommand(unsigned short, unsigned short, HWND);
    long                                OnNotify(WPARAM, NMHDR*);
	long								OnSize(unsigned int, unsigned int, unsigned int);

private:
    void                                CreateColumns();
    void                                OnPlaylistItemInsert(const CPlaylist::ITEM&);
    void                                OnPlaylistItemUpdate(unsigned int, const CPlaylist::ITEM&);
    void                                OnPlaylistItemDelete(unsigned int);
    void                                OnPlaylistItemsClear();
    void                                AddItem(const TCHAR*, const TCHAR*);
    void                                ModifyItem(int, const TCHAR*, const TCHAR*);
	void								ExchangeItems(unsigned int, unsigned int);
	void								OnMoveUpButtonClick();
	void								OnMoveDownButtonClick();
    void                                OnAddButtonClick();
    void                                OnRemoveButtonClick();
    void                                OnSaveButtonClick();
    void                                OnPlaylistViewDblClick(NMITEMACTIVATE*);

	Framework::LayoutObjectPtr	        m_layout;

    CPlaylist&                          m_playlist;
    Framework::Win32::CListView*        m_playlistView;
	Framework::Win32::CButton*			m_moveUpButton;
	Framework::Win32::CButton*			m_moveDownButton;
	Framework::Win32::CButton*          m_addButton;
    Framework::Win32::CButton*          m_removeButton;
	Framework::Win32::CButton*          m_saveButton;
};

#endif
