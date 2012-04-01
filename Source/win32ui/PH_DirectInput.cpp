#include "PH_DirectInput.h"
#include "ControllerSettingsWnd.h"
#include "InputConfig.h"
#include "placeholder_def.h"

CPH_DirectInput::CPH_DirectInput(HWND hWnd)
: m_hWnd(hWnd)
, m_manager(NULL)
{
	m_hWnd = hWnd;
	Initialize();
}

CPH_DirectInput::~CPH_DirectInput()
{
	delete m_manager;
}

CPadHandler::FactoryFunction CPH_DirectInput::GetFactoryFunction(HWND hWnd)
{
	return std::bind(&CPH_DirectInput::PadHandlerFactory, hWnd);
}

CPadHandler* CPH_DirectInput::PadHandlerFactory(HWND hWnd)
{
	return new CPH_DirectInput(hWnd);
}

void CPH_DirectInput::Initialize()
{
	m_manager = new Framework::DirectInput::CManager();
	m_manager->CreateKeyboard(m_hWnd);
	m_manager->CreateJoysticks(m_hWnd);
}

Framework::DirectInput::CManager* CPH_DirectInput::GetManager() const
{
	return m_manager;
}

void CPH_DirectInput::Update(uint8* ram)
{
	CInputConfig::InputEventHandler eventHandler(std::bind(&CPH_DirectInput::ProcessEvents, this, 
		std::placeholders::_1, std::placeholders::_2, ram));
	m_manager->ProcessEvents(
		std::bind(&CInputConfig::TranslateInputEvent, &CInputConfig::GetInstance(), 
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::tr1::cref(eventHandler)));
}

void CPH_DirectInput::ProcessEvents(PS2::CControllerInfo::BUTTON button, uint32 value, uint8* ram)
{
	for(auto listenerIterator(std::begin(m_listeners)); 
		listenerIterator != std::end(m_listeners); listenerIterator++)
	{
		auto pListener(*listenerIterator);
		if(PS2::CControllerInfo::IsAxis(button))
		{
			pListener->SetAxisState(0, button, static_cast<uint8>((value & 0xFFFF) >> 8), ram);
		}
		else
		{
			pListener->SetButtonState(0, button, value ? true : false, ram);
		}
	}
}

Framework::Win32::CModalWindow* CPH_DirectInput::CreateSettingsDialog(HWND parent)
{
	return new CControllerSettingsWnd(parent, m_manager);
}

void CPH_DirectInput::OnSettingsDialogDestroyed()
{

}
