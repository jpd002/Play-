#include "PH_GenericInput.h"

void CPH_GenericInput::Update(uint8* ram)
{
	for(auto* listener : m_listeners)
	{
		for(unsigned int pad = 0; pad < CInputBindingManager::MAX_PADS; pad++)
		{
			for(unsigned int buttonIdx = 0; buttonIdx < PS2::CControllerInfo::MAX_BUTTONS; buttonIdx++)
			{
				auto button = static_cast<PS2::CControllerInfo::BUTTON>(buttonIdx);
				const auto& binding = m_bindingManager.GetBinding(pad, button);
				if(!binding) continue;
				uint32 value = binding->GetValue();
				if(PS2::CControllerInfo::IsAxis(button))
				{
					listener->SetAxisState(pad, button, value & 0xFF, ram);
				}
				else
				{
					listener->SetButtonState(pad, button, value != 0, ram);
				}
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
	return []() { return new CPH_GenericInput(); };
}
