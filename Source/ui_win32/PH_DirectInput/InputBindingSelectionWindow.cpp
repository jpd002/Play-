#include "InputBindingSelectionWindow.h"
#include "win32/Rect.h"
#include "layout/LayoutEngine.h"
#include "string_cast.h"

#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

using namespace PH_DirectInput;

CInputBindingSelectionWindow::CInputBindingSelectionWindow(
	HWND parent, CInputManager& inputManager, PS2::CControllerInfo::BUTTON button) 
: CModalWindow(parent)
, m_inputManager(inputManager)
, m_button(button)
, m_currentBindingLabel(NULL)
, m_isActive(false)
, m_selected(false)
, m_selectedValue(-1)
, m_selectedId(0)
, m_selectedDevice(GUID())
, m_directInputManagerHandlerId(0)
{
	std::tstring title = _T("Select new binding for ") + string_cast<std::tstring>(PS2::CControllerInfo::m_buttonName[m_button]);

	Create(WNDSTYLEEX, Framework::Win32::CDefaultWndClass::GetName(), title.c_str(), WNDSTYLE, Framework::Win32::CRect(0, 0, 400, 100), parent, NULL);
	SetClassPtr();

	const CInputManager::CBinding* binding = inputManager.GetBinding(button);
	m_currentBindingLabel = new Framework::Win32::CStatic(
		m_hWnd, 
		binding ? inputManager.GetBindingDescription(button).c_str() : _T("Unbound"), 
		SS_CENTER);

	m_layout = 
		Framework::VerticalLayoutContainer(
			Framework::CLayoutStretch::Create() +
			Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 21, m_currentBindingLabel) +
			Framework::CLayoutStretch::Create()
		);

	RefreshLayout();

	m_directInputManagerHandlerId = inputManager.GetDirectInputManager()->RegisterInputEventHandler(std::bind(
		&CInputBindingSelectionWindow::ProcessEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	SetTimer(m_hWnd, 0, 50, NULL);
}

CInputBindingSelectionWindow::~CInputBindingSelectionWindow()
{
	m_inputManager.GetDirectInputManager()->UnregisterInputEventHandler(m_directInputManagerHandlerId);
}

long CInputBindingSelectionWindow::OnActivate(unsigned int activationType, bool minimized, HWND window)
{
	m_isActive = activationType != WA_INACTIVE;
	return TRUE;
}

long CInputBindingSelectionWindow::OnTimer(WPARAM)
{
	if(m_selected)
	{
		if(m_selectedValue != -1)
		{
			m_inputManager.SetPovHatBinding(m_button, CInputManager::BINDINGINFO(m_selectedDevice, m_selectedId), m_selectedValue);
		}
		else
		{
			m_inputManager.SetSimpleBinding(m_button, CInputManager::BINDINGINFO(m_selectedDevice, m_selectedId));
		}
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
	if(m_inputManager.GetDirectInputManager()->GetDeviceObjectInfo(device, id, &objectInstance))
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
		else if(objectInstance.dwType & DIDFT_POV)
		{
			if(value == -1) return;
			if(((value / 100) % 90) != 0) return;
			m_selectedValue = value;
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
