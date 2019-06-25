#include "PH_Libretro_Input.h"

extern retro_input_poll_t g_input_poll_cb;
extern retro_input_state_t g_input_state_cb;
extern std::map<int, int> g_ds2_to_retro_btn_map;

void CPH_Libretro_Input::Update(uint8* ram)
{
	if(!g_input_poll_cb || !g_input_state_cb || g_ds2_to_retro_btn_map.size() == 0)
		return;

	g_input_poll_cb();

	for(auto* listener : m_listeners)
	{
		for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
		{
			auto button = static_cast<PS2::CControllerInfo::BUTTON>(i);
			auto itr = g_ds2_to_retro_btn_map.find(button);

			if(itr == g_ds2_to_retro_btn_map.end()) return;

			auto currentButtonId = static_cast<PS2::CControllerInfo::BUTTON>(i);
			if(PS2::CControllerInfo::IsAxis(currentButtonId))
			{
				int index;
				if(i < 2)
					index = RETRO_DEVICE_INDEX_ANALOG_LEFT;
				else
					index = RETRO_DEVICE_INDEX_ANALOG_RIGHT;

				float value = g_input_state_cb(0, RETRO_DEVICE_ANALOG, index, itr->second);
				uint8 val = static_cast<int8>((value / 0xFF) + 0.5) + 0x7F;
				listener->SetAxisState(0, currentButtonId, val, ram);
			}
			else
			{
				uint32 value = g_input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, itr->second);
				listener->SetButtonState(0, currentButtonId, value != 0, ram);
			}
		}
	}
}

CPadHandler::FactoryFunction CPH_Libretro_Input::GetFactoryFunction()
{
	return []() { return new CPH_Libretro_Input(); };
}
