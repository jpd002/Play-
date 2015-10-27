#include "PH_iOS.h"

CPH_iOS::CPH_iOS()
{
	memset(&m_buttonStates, 0, sizeof(m_buttonStates));
	memset(&m_axisStates, 0, sizeof(m_axisStates));
}

CPH_iOS::~CPH_iOS()
{
	
}

CPadHandler::FactoryFunction CPH_iOS::GetFactoryFunction()
{
	return [] () { return new CPH_iOS(); };
}

void CPH_iOS::Update(uint8* ram)
{
	for(auto& listener : m_listeners)
	{
		for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
		{
			auto button = static_cast<PS2::CControllerInfo::BUTTON>(i);
			if(PS2::CControllerInfo::IsAxis(button))
			{
				float buttonValue = ((m_axisStates[i] + 1.0f) / 2.0f) * 255.f;
				listener->SetAxisState(0, button, static_cast<uint8>(buttonValue), ram);
			}
			else
			{
				listener->SetButtonState(0, button, m_buttonStates[i], ram);
			}
		}
	}
}

void CPH_iOS::SetButtonState(PS2::CControllerInfo::BUTTON buttonId, bool pressed)
{
	m_buttonStates[buttonId] = pressed;
}

void CPH_iOS::SetAxisState(PS2::CControllerInfo::BUTTON buttonId, float value)
{
	m_axisStates[buttonId] = value;
}
