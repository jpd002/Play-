#include "PH_GenericInput.h"
#include <array>
#include <utility>

void CPH_GenericInput::Update(uint8* ram)
{
	using MotorPair = std::pair<uint8, uint8>;
	std::array<MotorPair, CInputBindingManager::MAX_PADS> motorInfo;
	for(auto* interface : m_interfaces)
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
					interface->SetAxisState(pad, button, value & 0xFF, ram);
				}
				else
				{
					interface->SetButtonState(pad, button, value != 0, ram);
				}
			}
			// Only Sio2 currently provides vibration information
			MotorPair motors{0, 0};
			interface->GetVibration(pad, motors.first, motors.second);
			auto& [largeMotor, smallMotor] = motorInfo[pad];
			largeMotor = std::max(motors.first, largeMotor);
			smallMotor = std::max(motors.second, smallMotor);
		}
	}

	for(unsigned int pad = 0; pad < CInputBindingManager::MAX_PADS; pad++)
	{
		auto motorBinding = m_bindingManager.GetMotorBinding(pad);
		if(!motorBinding) continue;
		auto& [largeMotor, smallMotor] = motorInfo[pad];
		motorBinding->ProcessEvent(largeMotor, smallMotor);
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
