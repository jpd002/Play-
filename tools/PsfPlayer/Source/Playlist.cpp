#include "Playlist.h"

CPlaylist::CPlaylist()
{

}

CPlaylist::~CPlaylist()
{

}

void CPlaylist::AddItem(const char* path, const CPsfTags& tags)
{
    PLAYLIST_ITEM item;
    item.title = tags.GetTagValue("title");
    item.length = CPsfTags::ConvertTimeString(tags.GetTagValue("length").c_str());
    m_playlistItems[path] = item;

    PlaylistItemMapIterator itemIterator(m_playlistItems.find(path));

    OnItemChange(itemIterator);
}

bool CPlaylist::DoesItemExist(const char* path)
{
    return true;
}
