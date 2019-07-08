#include "PH_Libretro_Input.h"

extern bool libretro_supports_bitmasks;
extern retro_input_poll_t g_input_poll_cb;
extern retro_input_state_t g_input_state_cb;
extern std::map<int, int> g_ds2_to_retro_btn_map;

void CPH_Libretro_Input::UpdateInputState()
{
	std::lock_guard<std::mutex> lock(m_input_mutex);
	g_input_poll_cb();

	if(libretro_supports_bitmasks)
	{
		m_btns_state = g_input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
	}
	else
	{
		m_btns_state = 0;
		for(unsigned int i = 0; i <= RETRO_DEVICE_ID_JOYPAD_R3; i++)
		{
			if(g_input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i))
				m_btns_state |= (1 << i);
		}
	}

	for(unsigned int i = 0; i <= PS2::CControllerInfo::ANALOG_RIGHT_Y + 1; i++)
	{
		auto currentButtonId = static_cast<PS2::CControllerInfo::BUTTON>(i);
		if(PS2::CControllerInfo::IsAxis(currentButtonId))
		{
			int index;
			if(i < 2)
				index = RETRO_DEVICE_INDEX_ANALOG_LEFT;
			else
				index = RETRO_DEVICE_INDEX_ANALOG_RIGHT;

			float value = g_input_state_cb(0, RETRO_DEVICE_ANALOG, index, g_ds2_to_retro_btn_map[currentButtonId]);
			uint8 val = static_cast<int8>((value / 0xFF) + 0.5) + 0x7F;
			if(val > 0x7F - 5 && val < 0x7F + 5)
				val = 0x7F;
			m_axis_btn_state[currentButtonId] = val;
		}
	}
}

void CPH_Libretro_Input::Update(uint8* ram)
{
	std::lock_guard<std::mutex> lock(m_input_mutex);
	for(auto* listener : m_listeners)
	{
		for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
		{
			auto currentButtonId = static_cast<PS2::CControllerInfo::BUTTON>(i);
			if(PS2::CControllerInfo::IsAxis(currentButtonId))
			{
				listener->SetAxisState(0, currentButtonId, m_axis_btn_state[currentButtonId], ram);
			}
			else
			{
				uint32 val = m_btns_state & (1 << g_ds2_to_retro_btn_map[currentButtonId]);
				listener->SetButtonState(0, currentButtonId, val != 0, ram);
			}
		}
	}
}

CPadHandler::FactoryFunction CPH_Libretro_Input::GetFactoryFunction()
{
	return []() { return new CPH_Libretro_Input(); };
}
