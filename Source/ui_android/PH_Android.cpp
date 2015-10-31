#include <cassert>
#include "PH_Android.h"

CPH_Android::CPH_Android()
{
	memset(&m_buttonStates, 0, sizeof(m_buttonStates));
	memset(&m_axisStates, 0, sizeof(m_axisStates));
}

CPH_Android::~CPH_Android()
{
	
}

CPadHandler::FactoryFunction CPH_Android::GetFactoryFunction()
{
	return [] () { return new CPH_Android(); };
}

void CPH_Android::Update(uint8* ram)
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

void CPH_Android::SetButtonState(uint32 buttonId, bool pressed)
{
	assert(buttonId < PS2::CControllerInfo::MAX_BUTTONS);
	m_buttonStates[buttonId] = pressed;
}

void CPH_Android::SetAxisState(uint32 buttonId, float value)
{
	assert(buttonId < PS2::CControllerInfo::MAX_BUTTONS);
	m_axisStates[buttonId] = value;
}
