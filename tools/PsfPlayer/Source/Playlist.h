#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include "PsfTags.h"
#include <boost/signal.hpp>

class CPlaylist
{
public:
    struct PLAYLIST_ITEM
    {
        std::wstring                title;
        double                      length;
    };

    typedef std::map<std::string, PLAYLIST_ITEM> PlaylistItemMap;
    typedef PlaylistItemMap::const_iterator PlaylistItemMapIterator;

    typedef boost::signal<void (const PlaylistItemMapIterator&)> OnItemChangeEvent;

                                    CPlaylist();
    virtual                         ~CPlaylist();

    void                            AddItem(const char*, const CPsfTags&);
    bool                            DoesItemExist(const char*);

    OnItemChangeEvent               OnItemChange;

private:
    PlaylistItemMap                 m_playlistItems;
};

#endif
