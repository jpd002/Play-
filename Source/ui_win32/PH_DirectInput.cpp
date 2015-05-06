#include "PH_DirectInput.h"
#include "PH_DirectInput/ControllerSettingsWnd.h"
#include "../AppConfig.h"

CPH_DirectInput::CPH_DirectInput(HWND hWnd)
: m_hWnd(hWnd)
, m_inputManager(hWnd, CAppConfig::GetInstance())
{

}

CPH_DirectInput::~CPH_DirectInput()
{

}

CPadHandler::FactoryFunction CPH_DirectInput::GetFactoryFunction(HWND hWnd)
{
	return std::bind(&CPH_DirectInput::PadHandlerFactory, hWnd);
}

CPadHandler* CPH_DirectInput::PadHandlerFactory(HWND hWnd)
{
	return new CPH_DirectInput(hWnd);
}

void CPH_DirectInput::Update(uint8* ram)
{
	for(auto listenerIterator(std::begin(m_listeners)); 
		listenerIterator != std::end(m_listeners); listenerIterator++)
	{
		auto& listener(*listenerIterator);
		for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
		{
			PS2::CControllerInfo::BUTTON button = static_cast<PS2::CControllerInfo::BUTTON>(i);
			uint32 value = m_inputManager.GetBindingValue(button);
			if(PS2::CControllerInfo::IsAxis(button))
			{
				listener->SetAxisState(0, button, static_cast<uint8>((value & 0xFFFF) >> 8), ram);
			}
			else
			{
				listener->SetButtonState(0, button, value ? true : false, ram);
			}
		}
	}
}

Framework::Win32::CModalWindow* CPH_DirectInput::CreateSettingsDialog(HWND parent)
{
	return new PH_DirectInput::CControllerSettingsWnd(parent, m_inputManager);
}

void CPH_DirectInput::OnSettingsDialogDestroyed()
{

}
