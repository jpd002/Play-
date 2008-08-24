#include "ControllerSettingsWnd.h"
#include "win32/Rect.h"
#include "InputConfig.h"
#include "InputBindingSelectionWindow.h"
#include <vector>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include "layout/LayoutEngine.h"
#include "string_cast.h"
#include "Types.h"

#define CLSNAME     _T("ContollerSettingsWnd")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace PS2;
using namespace Framework;
using namespace boost;

CControllerSettingsWnd::CControllerSettingsWnd(HWND parent, DirectInput::CManager* directInputManager) :
CModalWindow(parent),
m_directInputManager(directInputManager),
m_autoConfigButton(NULL),
m_bindingList(NULL),
m_samplingEnabled(true)
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

    Create(WNDSTYLEEX, CLSNAME, _T("Controller Settings"), WNDSTYLE, Win32::CRect(0, 0, 550, 400), parent, NULL);
	SetClassPtr();

    m_bindingList       = new Win32::CListView(m_hWnd, Win32::CRect(0, 0, 1, 1), LVS_REPORT | LVS_NOSORTHEADER);
    m_ok                = new Win32::CButton(_T("OK"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_cancel            = new Win32::CButton(_T("Cancel"), m_hWnd, Win32::CRect(0, 0, 1, 1));
    m_autoConfigButton  = new Win32::CButton(_T("Auto Config"), m_hWnd, Win32::CRect(0, 0, 1, 1));

    m_bindingList->SetExtendedListViewStyle(m_bindingList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

    m_layout = 
        VerticalLayoutContainer(
            Win32::CLayoutWindow::CreateCustomBehavior(100, 100, 1, 1, m_bindingList) +
            HorizontalLayoutContainer(
                Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_autoConfigButton) +
                CLayoutStretch::Create() +
                Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_ok) +
                Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_cancel)
            )
        );

    RefreshLayout();
    PopulateList();
    UpdateBindings();

    SetTimer(m_hWnd, NULL, 50, NULL);
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

long CControllerSettingsWnd::OnTimer()
{
    if(m_samplingEnabled)
    {
        CInputConfig::InputEventHandler eventHandler(bind(&CControllerSettingsWnd::InputEventHandler, this, _1, _2));
        m_directInputManager->ProcessEvents(
            bind(&CInputConfig::TranslateInputEvent, &CInputConfig::GetInstance(), _1, _2, _3, std::tr1::cref(eventHandler)));
    }
    return TRUE;
}

long CControllerSettingsWnd::OnCommand(unsigned short id, unsigned short cmd, HWND from)
{
    if(m_autoConfigButton && from == m_autoConfigButton->m_hWnd)
    {
        AutoConfigKeyboard();
    }
    if(m_ok && from == m_ok->m_hWnd)
    {
        CInputConfig::GetInstance().Save();
        Destroy();
    }
    if(m_cancel && from == m_cancel->m_hWnd)
    {
        CInputConfig::GetInstance().Load();
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

void CControllerSettingsWnd::AutoConfigKeyboard()
{
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::START,        CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_RETURN));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::SELECT,       CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_LSHIFT));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::DPAD_LEFT,    CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_LEFT));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::DPAD_RIGHT,   CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_RIGHT));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::DPAD_UP,      CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_UP));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::DPAD_DOWN,    CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_DOWN));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::SQUARE,       CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_A));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::CROSS,        CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_Z));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::TRIANGLE,     CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_S));
    CInputConfig::GetInstance().SetSimpleBinding(CControllerInfo::CIRCLE,       CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_X));

    UpdateBindings();
}

void CControllerSettingsWnd::InputEventHandler(CControllerInfo::BUTTON button, uint32 value)
{
    UpdateButtonValue(button, value);
}

void CControllerSettingsWnd::UpdateBindings()
{
    for(int i = 0; i < m_bindingList->GetItemCount(); i++)
    {
        CControllerInfo::BUTTON button = static_cast<CControllerInfo::BUTTON>(m_bindingList->GetItemData(i));
        tstring description = CInputConfig::GetInstance().GetBindingDescription(m_directInputManager, button);
        m_bindingList->SetItemText(i, 1, description.c_str());
    }
}

void CControllerSettingsWnd::UpdateButtonValue(CControllerInfo::BUTTON button, uint32 value)
{
    int listViewIndex = m_bindingList->FindItemData(button);
    if(listViewIndex == -1) return;
    if(CControllerInfo::IsAxis(button))
    {
        m_bindingList->SetItemText(listViewIndex, 2, lexical_cast<tstring>(value).c_str());
    }
    else
    {
        m_bindingList->SetItemText(listViewIndex, 2, value ? _T("pressed") : _T(""));
    }
}

void CControllerSettingsWnd::OnListItemDblClick()
{
    int selection = m_bindingList->GetSelection();
    if(selection == -1) return;
    m_samplingEnabled = false;
    {
        CControllerInfo::BUTTON button = static_cast<CControllerInfo::BUTTON>(m_bindingList->GetItemData(selection));
        if(button < CControllerInfo::MAX_BUTTONS)
        {
            CInputBindingSelectionWindow dialog(m_hWnd, m_directInputManager, button);
            dialog.DoModal();
            UpdateBindings();
        }
    }
    m_samplingEnabled = true;
}

void CControllerSettingsWnd::PopulateList()
{
    LVCOLUMN column;

    RECT rc = m_bindingList->GetClientRect();

    memset(&column, 0, sizeof(LVCOLUMN));
    column.pszText  = _T("Button");
    column.mask     = LVCF_TEXT | LVCF_WIDTH;
    column.cx       = 1 * rc.right / 5;
    m_bindingList->InsertColumn(0, &column);

    memset(&column, 0, sizeof(LVCOLUMN));
    column.pszText  = _T("Binding");
    column.mask     = LVCF_TEXT | LVCF_WIDTH;
    column.cx       = 3 * rc.right / 5;
    m_bindingList->InsertColumn(1, &column);

    memset(&column, 0, sizeof(LVCOLUMN));
    column.pszText  = _T("Current Value");
    column.mask     = LVCF_TEXT | LVCF_WIDTH;
    column.cx       = 1 * rc.right / 5;
    m_bindingList->InsertColumn(2, &column);

    for(int i = CControllerInfo::MAX_BUTTONS - 1; i >= 0; i--)
    {
        tstring text = string_cast<tstring>(CControllerInfo::m_buttonName[i]);
        LVITEM itm;
        memset(&itm, 0, sizeof(LVITEM));
        itm.mask		= LVIF_TEXT | LVIF_PARAM;
        itm.pszText		= const_cast<TCHAR*>(text.c_str());
        itm.lParam      = i;
        m_bindingList->InsertItem(&itm);
        UpdateButtonValue(static_cast<CControllerInfo::BUTTON>(i), 0);
    }
}
