#include "InputBindingSelectionWindow.h"
#include "win32/Rect.h"
#include "layout/LayoutEngine.h"
#include "string_cast.h"
#include "placeholder_def.h"

#define CLSNAME		_T("CInputBindingSelectionWindow")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

CInputBindingSelectionWindow::CInputBindingSelectionWindow(
	HWND parent, Framework::DirectInput::CManager* directInputManager, PS2::CControllerInfo::BUTTON button) 
: CModalWindow(parent)
, m_directInputManager(directInputManager)
, m_button(button)
, m_currentBindingLabel(NULL)
, m_isActive(false)
, m_selected(false)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&MakeWndClass(CLSNAME));
	}

	std::tstring title = _T("Select new binding for ") + string_cast<std::tstring>(PS2::CControllerInfo::m_buttonName[m_button]);

	m_binding = CInputConfig::GetInstance().GetBinding(button);

	Create(WNDSTYLEEX, CLSNAME, title.c_str(), WNDSTYLE, Framework::Win32::CRect(0, 0, 400, 100), parent, NULL);
	SetClassPtr();

	m_currentBindingLabel = new Framework::Win32::CStatic(
		m_hWnd, 
		m_binding ? m_binding->GetDescription(m_directInputManager).c_str() : _T("Unbound"), 
		SS_CENTER);

	m_layout = 
		Framework::VerticalLayoutContainer(
			Framework::CLayoutStretch::Create() +
			Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 21, m_currentBindingLabel) +
			Framework::CLayoutStretch::Create()
		);

	RefreshLayout();
	SetTimer(m_hWnd, 0, 16, NULL);
}

CInputBindingSelectionWindow::~CInputBindingSelectionWindow()
{

}

long CInputBindingSelectionWindow::OnActivate(unsigned int activationType, bool minimized, HWND window)
{
	m_isActive = activationType != WA_INACTIVE;
	return TRUE;
}

long CInputBindingSelectionWindow::OnTimer(WPARAM)
{
	m_directInputManager->ProcessEvents(std::bind(&CInputBindingSelectionWindow::ProcessEvent, this, 
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	if(m_selected)
	{
		CInputConfig::GetInstance().SetSimpleBinding(m_button, CInputConfig::BINDINGINFO(m_selectedDevice, m_selectedId));
		Destroy();
	}
	return TRUE;
}

void CInputBindingSelectionWindow::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_layout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_layout->RefreshGeometry();

	Redraw();
}

void CInputBindingSelectionWindow::ProcessEvent(const GUID& device, uint32 id, uint32 value)
{
	if(!m_isActive) return;
	if(m_selected) return;
	DIDEVICEOBJECTINSTANCE objectInstance;
	if(m_directInputManager->GetDeviceObjectInfo(device, id, &objectInstance))
	{
		if(objectInstance.dwType & DIDFT_AXIS)
		{
			if(!PS2::CControllerInfo::IsAxis(m_button)) return;
			float axisValue = static_cast<float>(static_cast<int16>(value - 0x7FFF)) / 32768.f;
			if(abs(axisValue) < 0.85)
			{
				return;
			}
		}
		else if(objectInstance.dwType & DIDFT_BUTTON)
		{
			if(PS2::CControllerInfo::IsAxis(m_button)) return;
			if(!value) return;
		}
		else
		{
			return;
		}
		m_selectedDevice = device;
		m_selectedId = id;
		m_selected = true;
	}
}
