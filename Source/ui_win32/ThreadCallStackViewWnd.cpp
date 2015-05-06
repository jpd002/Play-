#include "ThreadCallStackViewWnd.h"
#include "layout/LayoutEngine.h"
#include "resource.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"
#include "DebugUtils.h"

CThreadCallStackViewWnd::CThreadCallStackViewWnd(HWND parentWindow) 
: CDialog(MAKEINTRESOURCE(IDD_DEBUG_THREADCALLSTACK), parentWindow)
, m_hasSelection(false)
, m_selectedAddress(0)
{
	SetClassPtr();

	m_okButton			= new Framework::Win32::CButton(GetItem(IDOK));
	m_cancelButton		= new Framework::Win32::CButton(GetItem(IDCANCEL));
	m_callStackItemList = new Framework::Win32::CListBox(GetItem(IDC_CALLSTACKITEM_LIST));

	RECT buttonSize;
	SetRect(&buttonSize, 0, 0, 75, 16);
	MapDialogRect(m_hWnd, &buttonSize);
	unsigned int buttonWidth = buttonSize.right - buttonSize.left;
	unsigned int buttonHeight = buttonSize.bottom - buttonSize.top;

	m_layout = 
		Framework::VerticalLayoutContainer
		(
			Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateCustomBehavior(100, 20, 1, 1, m_callStackItemList)) +
			Framework::HorizontalLayoutContainer
			(
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(buttonWidth, buttonHeight, m_okButton)) +
				Framework::LayoutExpression(Framework::CLayoutStretch::Create()) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(buttonWidth, buttonHeight, m_cancelButton))
			)
		);

	{
		RECT rc = GetClientRect();
		m_layout->SetRect(rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);
		m_layout->RefreshGeometry();
	}
}

CThreadCallStackViewWnd::~CThreadCallStackViewWnd()
{

}

bool CThreadCallStackViewWnd::HasSelection() const
{
	return m_hasSelection;
}

uint32 CThreadCallStackViewWnd::GetSelectedAddress() const
{
	return m_selectedAddress;
}

void CThreadCallStackViewWnd::SetItems(CMIPS* context, const CMIPSAnalysis::CallStackItemArray& items, const BiosDebugModuleInfoArray& modules)
{
	for(auto itemIterator(std::begin(items));
		itemIterator != std::end(items); itemIterator++)
	{
		const auto& item(*itemIterator);
		std::tstring locationString = DebugUtils::PrintAddressLocation(item, context, modules);
		unsigned int itemIndex = m_callStackItemList->AddString(locationString.c_str());
		m_callStackItemList->SetItemData(itemIndex, item);
	}

	m_callStackItemList->SetCurrentSelection(0);
}

void CThreadCallStackViewWnd::ProcessSelection()
{
	int selectedItem = m_callStackItemList->GetCurrentSelection();
	if(selectedItem == LB_ERR) return;
	m_selectedAddress = static_cast<uint32>(m_callStackItemList->GetItemData(selectedItem));
	m_hasSelection = true;
	Destroy();
}

long CThreadCallStackViewWnd::OnCommand(unsigned short id, unsigned short msg, HWND fromWnd)
{
	switch(id)
	{
	case IDC_CALLSTACKITEM_LIST:
		switch(msg)
		{
		case LBN_DBLCLK:
			ProcessSelection();
			break;
		}
		break;
	case IDOK:
		ProcessSelection();
		break;
	case IDCANCEL:
		Destroy();
		break;
	}
	return FALSE;
}
