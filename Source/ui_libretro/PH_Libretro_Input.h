#pragma once

#include <map>
#include <mutex>
#include "PadHandler.h"
// #include "InputBindingManager.h"
#include "libretro.h"

class CPH_Libretro_Input : public CPadHandler
{
public:
	CPH_Libretro_Input() = default;
	virtual ~CPH_Libretro_Input() = default;

	void Update(uint8*) override;

	static FactoryFunction GetFactoryFunction();

	void UpdateInputState();

private:
	int16 m_btns_state = 0;
	uint8 m_axis_btn_state[4] = {0x7F};
	std::mutex m_input_mutex;
};
