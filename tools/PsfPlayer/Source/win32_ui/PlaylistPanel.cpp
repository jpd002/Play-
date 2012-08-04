#include "PlaylistPanel.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "TimeToString.h"
#include "string_cast.h"
#include "layout/LayoutEngine.h"
#include "resource.h"

CPlaylistPanel::CPlaylistPanel(HWND parentWnd, CPlaylist& playlist)
: Framework::Win32::CDialog(MAKEINTRESOURCE(IDD_PLAYLISTPANEL), parentWnd)
, m_playlistView(NULL)
, m_moveUpButton(NULL)
, m_moveDownButton(NULL)
, m_addButton(NULL)
, m_removeButton(NULL)
, m_saveButton(NULL)
, m_playlist(playlist)
{
	SetClassPtr();

	m_playlistView = new Framework::Win32::CListView(GetItem(IDC_PLAYLIST_LISTVIEW));
	m_playlistView->SetExtendedListViewStyle(m_playlistView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_moveUpButton		= new Framework::Win32::CButton(GetItem(IDC_UP_BUTTON));
	m_moveDownButton	= new Framework::Win32::CButton(GetItem(IDC_DOWN_BUTTON));
	m_addButton			= new Framework::Win32::CButton(GetItem(IDC_ADD_BUTTON));
	m_removeButton		= new Framework::Win32::CButton(GetItem(IDC_REMOVE_BUTTON));
	m_saveButton		= new Framework::Win32::CButton(GetItem(IDC_SAVE_BUTTON));

	RECT buttonSize;
	SetRect(&buttonSize, 0, 0, 16, 16);
	MapDialogRect(m_hWnd, &buttonSize);

	unsigned int buttonWidth = buttonSize.right - buttonSize.left;
	unsigned int buttonHeight = buttonSize.bottom - buttonSize.top;

	m_layout = 
		Framework::VerticalLayoutContainer(
			Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateCustomBehavior(300, 200, 1, 1, m_playlistView)) +
			Framework::HorizontalLayoutContainer(
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(buttonWidth, buttonHeight, m_moveUpButton)) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(buttonWidth, buttonHeight, m_moveDownButton)) +
				Framework::LayoutExpression(Framework::CLayoutStretch::Create()) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(buttonWidth, buttonHeight, m_addButton)) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(buttonWidth, buttonHeight, m_removeButton)) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(buttonWidth, buttonHeight, m_saveButton))
			)
		);

	CreateColumns();

	m_playlist.OnItemInsert.connect(std::tr1::bind(&CPlaylistPanel::OnPlaylistItemInsert, this, std::tr1::placeholders::_1));
	m_playlist.OnItemUpdate.connect(std::tr1::bind(&CPlaylistPanel::OnPlaylistItemUpdate, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
	m_playlist.OnItemDelete.connect(std::tr1::bind(&CPlaylistPanel::OnPlaylistItemDelete, this, std::tr1::placeholders::_1));
	m_playlist.OnItemsClear.connect(std::tr1::bind(&CPlaylistPanel::OnPlaylistItemsClear, this));

	RefreshLayout();
}

CPlaylistPanel::~CPlaylistPanel()
{

}

long CPlaylistPanel::OnCommand(unsigned short cmd, unsigned short id, HWND sender)
{
	if(CWindow::IsCommandSource(m_moveUpButton, sender))
	{
		OnMoveUpButtonClick();
	}
	else if(CWindow::IsCommandSource(m_moveDownButton, sender))
	{
		OnMoveDownButtonClick();
	}
	else if(CWindow::IsCommandSource(m_saveButton, sender))
	{
		OnSaveButtonClick();
	}
	else if(CWindow::IsCommandSource(m_addButton, sender))
	{
		OnAddButtonClick();
	}
	else if(CWindow::IsCommandSource(m_removeButton, sender))
	{
		OnRemoveButtonClick();
	}
	return FALSE;
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

long CPlaylistPanel::OnSize(unsigned int, unsigned int, unsigned int)
{
	RefreshLayout();
	return TRUE;
}

void CPlaylistPanel::OnPlaylistViewDblClick(NMITEMACTIVATE* itemActivate)
{
	if(itemActivate->iItem == -1) return;
	OnItemDblClick(itemActivate->iItem);
}

void CPlaylistPanel::OnMoveUpButtonClick()
{
	int selection = m_playlistView->GetSelection();
	if(selection == -1) return;
	if(selection == 0) return;
	ExchangeItems(selection - 1, selection);
}

void CPlaylistPanel::OnMoveDownButtonClick()
{
	int selection = m_playlistView->GetSelection();
	if(selection == -1) return;
	if(selection == m_playlistView->GetItemCount() - 1) return;
	ExchangeItems(selection + 1, selection);
}

void CPlaylistPanel::OnAddButtonClick()
{
	OnAddClick();
}

void CPlaylistPanel::OnRemoveButtonClick()
{
	int selection = m_playlistView->GetSelection();
	if(selection == -1) return;
	OnRemoveClick(selection);
}

void CPlaylistPanel::OnSaveButtonClick()
{
	OnSaveClick();
}

void CPlaylistPanel::AddItem(const TCHAR* title, const TCHAR* length)
{
	LVITEM item;
	memset(&item, 0, sizeof(LVITEM));
	item.pszText	= const_cast<TCHAR*>(title);
	item.iItem		= m_playlistView->GetItemCount();
	item.mask		= LVIF_TEXT;

	int itemIdx = m_playlistView->InsertItem(&item);
	m_playlistView->SetItemText(itemIdx, 1, length);
}

void CPlaylistPanel::ModifyItem(int index, const TCHAR* title, const TCHAR* length)
{
	assert(index < m_playlistView->GetItemCount());
	if(index >= m_playlistView->GetItemCount())
	{
		throw std::runtime_error("Invalid item index to update.");
	}

	m_playlistView->SetItemText(index, 0, title);
	m_playlistView->SetItemText(index, 1, length);
}

void CPlaylistPanel::ExchangeItems(unsigned int dst, unsigned int src)
{
	m_playlist.ExchangeItems(dst, src);
	m_playlistView->SetSelection(dst);
	m_playlistView->SetItemState(src,	0,								LVIS_FOCUSED | LVIS_SELECTED);
	m_playlistView->SetItemState(dst,	LVIS_FOCUSED | LVIS_SELECTED,	LVIS_FOCUSED | LVIS_SELECTED);
	m_playlistView->EnsureItemVisible(dst, true);
}

void CPlaylistPanel::CreateColumns()
{
	LVCOLUMN column;
	memset(&column, 0, sizeof(LVCOLUMN));
	column.pszText	= _T("Title");
	column.mask		= LVCF_TEXT;
	m_playlistView->InsertColumn(0, &column);

	memset(&column, 0, sizeof(LVCOLUMN));
	column.pszText	= _T("Length");
	column.mask		= LVCF_TEXT;
	m_playlistView->InsertColumn(1, &column);
}

void CPlaylistPanel::OnPlaylistItemInsert(const CPlaylist::ITEM& item)
{
	std::tstring title = string_cast<std::tstring>(item.title);
	std::tstring length = TimeToString<std::tstring>(item.length);
	AddItem(title.c_str(), length.c_str());
}

void CPlaylistPanel::OnPlaylistItemUpdate(unsigned int index, const CPlaylist::ITEM& item)
{
	std::tstring title = string_cast<std::tstring>(item.title);
	std::tstring length = TimeToString<std::tstring>(item.length);
	ModifyItem(index, title.c_str(), length.c_str());
}

void CPlaylistPanel::OnPlaylistItemDelete(unsigned int index)
{
	assert(index < static_cast<unsigned int>(m_playlistView->GetItemCount()));
	if(index >= static_cast<unsigned int>(m_playlistView->GetItemCount()))
	{
		throw std::runtime_error("Invalid item index to delete.");
	}

	m_playlistView->DeleteItem(index);
}

void CPlaylistPanel::OnPlaylistItemsClear()
{
	m_playlistView->DeleteAllItems();
}

void CPlaylistPanel::RefreshLayout()
{
	//Resize layout
	{
		RECT rc = GetClientRect();
		m_layout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
		m_layout->RefreshGeometry();
	}

	//Resize columns
	{
		RECT clientRect = m_playlistView->GetClientRect();

		m_playlistView->SetColumnWidth(0, 3 * clientRect.right / 4);
		m_playlistView->SetColumnWidth(1, 1 * clientRect.right / 4);
	}
}
