#include "PH_GenericInput.h"

void CPH_GenericInput::Update(uint8* ram)
{
	for(auto* listener : m_listeners)
	{
		for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
		{
			auto button = static_cast<PS2::CControllerInfo::BUTTON>(i);
			const auto& binding = m_bindingManager.GetBinding(0, button);
			if(!binding) continue;
			uint32 value = binding->GetValue();
			auto currentButtonId = static_cast<PS2::CControllerInfo::BUTTON>(i);
			if(PS2::CControllerInfo::IsAxis(currentButtonId))
			{
				listener->SetAxisState(0, currentButtonId, value & 0xFF, ram);
			}
			else
			{
				listener->SetButtonState(0, currentButtonId, value != 0, ram);
			}
		}
	}
}

CInputBindingManager& CPH_GenericInput::GetBindingManager()
{
	return m_bindingManager;
}

CPadHandler::FactoryFunction CPH_GenericInput::GetFactoryFunction()
{
	return [] () { return new CPH_GenericInput(); };
}
