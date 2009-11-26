#ifndef _PLAYLISTPANEL_H_
#define _PLAYLISTPANEL_H_

#include "Panel.h"
#include "Playlist.h"
#include "win32/ListView.h"
#include <boost/bimap.hpp>

class CPlaylistPanel : public CPanel, public boost::signals::trackable
{
public:
    typedef boost::signal<void (const char*)> OnItemDblClickEvent;

                                        CPlaylistPanel(HWND, CPlaylist&);
    virtual                             ~CPlaylistPanel();

    void                                RefreshLayout();

    OnItemDblClickEvent                 OnItemDblClick;

protected:
    long                                OnNotify(WPARAM, NMHDR*);

private:
    typedef boost::bimap<std::string, unsigned int> ListViewKeyMap;

    void                                CreateColumns();
    void                                OnPlaylistItemChange(const CPlaylist::PlaylistItemMapIterator&);
    void                                AddItem(uint32, const TCHAR*, const TCHAR*);
    void                                ModifyItem(uint32, const TCHAR*, const TCHAR*);
    void                                OnPlaylistViewDblClick(NMITEMACTIVATE*);

    CPlaylist&                          m_playlist;
    Framework::Win32::CListView*        m_playlistView;
    ListViewKeyMap                      m_listViewKeys;

    uint32                              m_nextListViewKey;
};

#endif
