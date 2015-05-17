#include "PH_Android.h"
#include "com_virtualapplications_play_VirtualPadConstants.h"

CPH_Android::CPH_Android()
{
	memset(&m_buttonStates, 0, sizeof(m_buttonStates));
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
			if(PS2::CControllerInfo::IsAxis(button)) continue;
			listener->SetButtonState(0, button, m_buttonStates[i], ram);
		}
	}
}

void CPH_Android::ReportInput(uint32 buttonId, bool pressed)
{
	switch(buttonId)
	{
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_START:
		m_buttonStates[PS2::CControllerInfo::START] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_SELECT:
		m_buttonStates[PS2::CControllerInfo::SELECT] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_UP:
		m_buttonStates[PS2::CControllerInfo::DPAD_UP] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_DOWN:
		m_buttonStates[PS2::CControllerInfo::DPAD_DOWN] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_LEFT:
		m_buttonStates[PS2::CControllerInfo::DPAD_LEFT] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_RIGHT:
		m_buttonStates[PS2::CControllerInfo::DPAD_RIGHT] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_TRIANGLE:
		m_buttonStates[PS2::CControllerInfo::TRIANGLE] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_CIRCLE:
		m_buttonStates[PS2::CControllerInfo::CIRCLE] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_SQUARE:
		m_buttonStates[PS2::CControllerInfo::SQUARE] = pressed;
		break;
	case com_virtualapplications_play_VirtualPadConstants_BUTTON_CROSS:
		m_buttonStates[PS2::CControllerInfo::CROSS] = pressed;
		break;
	}
}
