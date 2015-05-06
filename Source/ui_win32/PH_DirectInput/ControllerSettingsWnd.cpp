#include "ControllerSettingsWnd.h"
#include "win32/Rect.h"
#include "InputBindingSelectionWindow.h"
#include <vector>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include "layout/LayoutEngine.h"
#include "string_cast.h"
#include "Types.h"
#include "placeholder_def.h"

#define CLSNAME		_T("ContollerSettingsWnd")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)
#define SCALE(x)	MulDiv(x, ydpi, 96)

using namespace PH_DirectInput;

CControllerSettingsWnd::CControllerSettingsWnd(HWND parent, CInputManager& inputManager)
: CModalWindow(parent)
, m_inputManager(inputManager)
, m_autoConfigButton(NULL)
, m_bindingList(NULL)
, m_valuesCached(false)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		wc.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&wc);
	}

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);

	Create(WNDSTYLEEX, CLSNAME, _T("Controller Settings"), WNDSTYLE, Framework::Win32::CRect(0, 0, SCALE(550), SCALE(400)), parent, NULL);
	SetClassPtr();

	m_bindingList		= new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT | LVS_NOSORTHEADER);
	m_ok				= new Framework::Win32::CButton(_T("OK"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_cancel			= new Framework::Win32::CButton(_T("Cancel"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_autoConfigButton	= new Framework::Win32::CButton(_T("Auto Config"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));

	m_bindingList->SetExtendedListViewStyle(m_bindingList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_layout = 
		Framework::VerticalLayoutContainer(
		Framework::Win32::CLayoutWindow::CreateCustomBehavior(SCALE(100), SCALE(100), 1, 1, m_bindingList) +
			Framework::HorizontalLayoutContainer(
			Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_autoConfigButton) +
				Framework::CLayoutStretch::Create() +
				Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_ok) +
				Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_cancel)
			)
		);

	RefreshLayout();
	PopulateList();
	UpdateBindings();
	UpdateBindingValues();

	SetTimer(m_hWnd, 0, 16, NULL);
}

CControllerSettingsWnd::~CControllerSettingsWnd()
{

}

void CControllerSettingsWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_layout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_layout->RefreshGeometry();

	Redraw();
}

long CControllerSettingsWnd::OnCommand(unsigned short id, unsigned short cmd, HWND from)
{
	if(m_autoConfigButton && from == m_autoConfigButton->m_hWnd)
	{
		AutoConfigKeyboard();
	}
	if(CWindow::IsCommandSource(m_ok, from))
	{
		m_inputManager.Save();
		Destroy();
	}
	if(CWindow::IsCommandSource(m_cancel, from))
	{
		m_inputManager.Load();
		Destroy();
	}
	return TRUE;
}

long CControllerSettingsWnd::OnNotify(WPARAM param, NMHDR* header)
{
	if(m_bindingList && m_bindingList->m_hWnd == header->hwndFrom)
	{
		if(header->code == NM_DBLCLK)
		{
			OnListItemDblClick();
		}
	}
	return FALSE;
}

long CControllerSettingsWnd::OnTimer(WPARAM)
{
	UpdateBindingValues();
	return FALSE;
}

void CControllerSettingsWnd::AutoConfigKeyboard()
{
	m_inputManager.AutoConfigureKeyboard();
	UpdateBindings();
}

void CControllerSettingsWnd::UpdateBindings()
{
	for(int i = 0; i < m_bindingList->GetItemCount(); i++)
	{
		PS2::CControllerInfo::BUTTON button = static_cast<PS2::CControllerInfo::BUTTON>(m_bindingList->GetItemData(i));
		std::tstring description = m_inputManager.GetBindingDescription(button);
		m_bindingList->SetItemText(i, 1, description.c_str());
	}
}

void CControllerSettingsWnd::UpdateBindingValues()
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		PS2::CControllerInfo::BUTTON button = static_cast<PS2::CControllerInfo::BUTTON>(m_bindingList->GetItemData(i));
		int listViewIndex = m_bindingList->FindItemData(button);
		if(listViewIndex == -1) continue;
		uint32 value = m_inputManager.GetBindingValue(button);
		if(m_valuesCached && m_cachedValues[button] == value) continue;
		m_cachedValues[button] = value;
		if(PS2::CControllerInfo::IsAxis(button))
		{
			m_bindingList->SetItemText(listViewIndex, 2, boost::lexical_cast<std::tstring>(value).c_str());
		}
		else
		{
			m_bindingList->SetItemText(listViewIndex, 2, value ? _T("pressed") : _T(""));
		}
	}
	m_valuesCached = true;
}

void CControllerSettingsWnd::OnListItemDblClick()
{
	int selection = m_bindingList->GetSelection();
	if(selection == -1) return;
	{
		PS2::CControllerInfo::BUTTON button = static_cast<PS2::CControllerInfo::BUTTON>(m_bindingList->GetItemData(selection));
		if(button < PS2::CControllerInfo::MAX_BUTTONS)
		{
			CInputBindingSelectionWindow dialog(m_hWnd, m_inputManager, button);
			dialog.DoModal();
			UpdateBindings();
		}
	}
}

void CControllerSettingsWnd::PopulateList()
{
	LVCOLUMN column;

	RECT rc = m_bindingList->GetClientRect();

	memset(&column, 0, sizeof(LVCOLUMN));
	column.pszText	= _T("Button");
	column.mask		= LVCF_TEXT | LVCF_WIDTH;
	column.cx		= 1 * rc.right / 5;
	m_bindingList->InsertColumn(0, column);

	memset(&column, 0, sizeof(LVCOLUMN));
	column.pszText	= _T("Binding");
	column.mask		= LVCF_TEXT | LVCF_WIDTH;
	column.cx		= 3 * rc.right / 5;
	m_bindingList->InsertColumn(1, column);

	memset(&column, 0, sizeof(LVCOLUMN));
	column.pszText	= _T("Current Value");
	column.mask		= LVCF_TEXT | LVCF_WIDTH;
	column.cx		= 1 * rc.right / 5;
	m_bindingList->InsertColumn(2, column);

	for(int i = PS2::CControllerInfo::MAX_BUTTONS - 1; i >= 0; i--)
	{
		std::tstring text = string_cast<std::tstring>(PS2::CControllerInfo::m_buttonName[i]);
		LVITEM itm;
		memset(&itm, 0, sizeof(LVITEM));
		itm.mask		= LVIF_TEXT | LVIF_PARAM;
		itm.pszText		= const_cast<TCHAR*>(text.c_str());
		itm.lParam		= i;
		m_bindingList->InsertItem(itm);
	}
}
