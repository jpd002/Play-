#include "PH_Android.h"
#include "com_virtualapplications_play_InputManagerConstants.h"

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
	switch(buttonId)
	{
	case com_virtualapplications_play_InputManagerConstants_BUTTON_START:
		m_buttonStates[PS2::CControllerInfo::START] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_SELECT:
		m_buttonStates[PS2::CControllerInfo::SELECT] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_UP:
		m_buttonStates[PS2::CControllerInfo::DPAD_UP] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_DOWN:
		m_buttonStates[PS2::CControllerInfo::DPAD_DOWN] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_LEFT:
		m_buttonStates[PS2::CControllerInfo::DPAD_LEFT] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_RIGHT:
		m_buttonStates[PS2::CControllerInfo::DPAD_RIGHT] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_TRIANGLE:
		m_buttonStates[PS2::CControllerInfo::TRIANGLE] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_CIRCLE:
		m_buttonStates[PS2::CControllerInfo::CIRCLE] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_SQUARE:
		m_buttonStates[PS2::CControllerInfo::SQUARE] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_CROSS:
		m_buttonStates[PS2::CControllerInfo::CROSS] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_L1:
		m_buttonStates[PS2::CControllerInfo::L1] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_L2:
		m_buttonStates[PS2::CControllerInfo::L2] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_R1:
		m_buttonStates[PS2::CControllerInfo::R1] = pressed;
		break;
	case com_virtualapplications_play_InputManagerConstants_BUTTON_R2:
		m_buttonStates[PS2::CControllerInfo::R2] = pressed;
		break;
	}
}

void CPH_Android::SetAxisState(uint32 buttonId, float value)
{
	switch(buttonId)
	{
	case com_virtualapplications_play_InputManagerConstants_ANALOG_LEFT_X:
		m_axisStates[PS2::CControllerInfo::ANALOG_LEFT_X] = value;
		break;
	case com_virtualapplications_play_InputManagerConstants_ANALOG_LEFT_Y:
		m_axisStates[PS2::CControllerInfo::ANALOG_LEFT_Y] = value;
		break;
	case com_virtualapplications_play_InputManagerConstants_ANALOG_RIGHT_X:
		m_axisStates[PS2::CControllerInfo::ANALOG_RIGHT_X] = value;
		break;
	case com_virtualapplications_play_InputManagerConstants_ANALOG_RIGHT_Y:
		m_axisStates[PS2::CControllerInfo::ANALOG_RIGHT_Y] = value;
		break;
	}
}
