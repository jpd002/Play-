#include "PlaylistPanel.h"
#include "win32/Rect.h"
#include "TimeToString.h"
#include "string_cast.h"

#define CLSNAME			                _T("PlaylistPanel")
#define WNDSTYLE		                (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CHILD)
#define WNDSTYLEEX		                (0)
#define WM_UPDATEVIS	                (WM_USER + 1)

using namespace Framework;

CPlaylistPanel::CPlaylistPanel(HWND parentWnd, CPlaylist& playlist)
: m_playlistView(NULL)
, m_playlist(playlist)
, m_nextListViewKey(0)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&Win32::CWindow::MakeWndClass(CLSNAME));
	}

    Create(WNDSTYLEEX, CLSNAME, _T(""), WNDSTYLE, Win32::CRect(0, 0, 1, 1), parentWnd, NULL);
    SetClassPtr();

    m_playlistView = new Win32::CListView(m_hWnd, Win32::CRect(0, 0, 1, 1), LVS_REPORT | LVS_SORTASCENDING);
	m_playlistView->SetExtendedListViewStyle(m_playlistView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

    CreateColumns();

    m_playlist.OnItemChange.connect(std::tr1::bind(&CPlaylistPanel::OnPlaylistItemChange, this, std::tr1::placeholders::_1));

    m_nextListViewKey = 0;
}

CPlaylistPanel::~CPlaylistPanel()
{

}

long CPlaylistPanel::OnNotify(WPARAM wParam, NMHDR* hdr)
{
    if(CWindow::IsNotifySource(m_playlistView, hdr))
    {
        switch(hdr->code)
        {
        case NM_DBLCLK:
            OnPlaylistViewDblClick(reinterpret_cast<NMITEMACTIVATE*>(hdr));
            break;
        }
    }
    return FALSE;
}

void CPlaylistPanel::OnPlaylistViewDblClick(NMITEMACTIVATE* itemActivate)
{
    if(itemActivate->iItem == -1) return;
    uint32 param = m_playlistView->GetItemData(itemActivate->iItem);
    ListViewKeyMap::right_const_iterator keyIterator = m_listViewKeys.right.find(param);
    assert(keyIterator != m_listViewKeys.right.end());
    if(keyIterator != m_listViewKeys.right.end())
    {
        OnItemDblClick(keyIterator->second.c_str());
    }
}

void CPlaylistPanel::AddItem(uint32 key, const TCHAR* title, const TCHAR* length)
{
    LVITEM item;
    memset(&item, 0, sizeof(LVITEM));
    item.pszText    = const_cast<TCHAR*>(title);
    item.lParam     = key;
    item.mask       = LVIF_TEXT | LVIF_PARAM;
    int itemIdx = m_playlistView->InsertItem(&item);

//    m_playlist->SetItemText(itemIndex, 1, TimeToString<std::tstring>(length).c_str());
    m_playlistView->SetItemText(itemIdx, 1, length);
}

void CPlaylistPanel::ModifyItem(uint32 key, const TCHAR* title, const TCHAR* length)
{
    int itemIdx = m_playlistView->FindItemData(key);
    assert(itemIdx != -1);
    if(itemIdx == -1) return;

    m_playlistView->SetItemText(itemIdx, 0, title);
    m_playlistView->SetItemText(itemIdx, 1, length);
}

void CPlaylistPanel::CreateColumns()
{
    LVCOLUMN column;
    memset(&column, 0, sizeof(LVCOLUMN));
    column.pszText  = _T("Title");
    column.mask     = LVCF_TEXT;
    m_playlistView->InsertColumn(0, &column);

    memset(&column, 0, sizeof(LVCOLUMN));
    column.pszText  = _T("Length");
    column.mask     = LVCF_TEXT;
    m_playlistView->InsertColumn(1, &column);
}

void CPlaylistPanel::OnPlaylistItemChange(const CPlaylist::PlaylistItemMapIterator& itemIterator)
{
    const CPlaylist::PLAYLIST_ITEM& item(itemIterator->second);

    std::tstring title, length;
    title = string_cast<std::tstring>(item.title);
    length = TimeToString<std::tstring>(item.length);

    ListViewKeyMap::left_const_iterator keyIterator(m_listViewKeys.left.find(itemIterator->first));
    if(keyIterator == m_listViewKeys.left.end())
    {
        //Element
        uint32 newKey = ++m_nextListViewKey;
        m_listViewKeys.insert(ListViewKeyMap::value_type(itemIterator->first, newKey));
        AddItem(newKey, title.c_str(), length.c_str());
    }
    else
    {
        uint32 key = keyIterator->second;
        ModifyItem(key, title.c_str(), length.c_str());
    }
}

void CPlaylistPanel::RefreshLayout()
{
    //Resize panel
    {
        Win32::CRect clientRect(0, 0, 0, 0);
        ::GetClientRect(GetParent(), clientRect);
        SetSizePosition(clientRect);
    }

    //Resize list view
    {
        Win32::CRect clientRect(0, 0, 0, 0);
        GetClientRect(clientRect);
        m_playlistView->SetSizePosition(clientRect);
    }

    //Resize columns
    {
        RECT clientRect = m_playlistView->GetClientRect();

        m_playlistView->SetColumnWidth(0, 3 * clientRect.right / 4);
        m_playlistView->SetColumnWidth(1, 1 * clientRect.right / 4);
    }
}
