#pragma once

#include <vector>
#include <string>
#include <map>
#include "Types.h"
#include "ControllerInfo.h"

struct ARCADE_MACHINE_DEF
{
	enum class INPUT_MODE
	{
		DEFAULT,
		LIGHTGUN,
		DRUM,
		DRIVE,
	};

	enum DRIVER
	{
		UNKNOWN,
		NAMCO_SYSTEM_246,
		NAMCO_SYSTEM_147,
	};

	struct PATCH
	{
		uint32 address = 0;
		uint32 value = 0;
	};

	std::string id;
	std::string parent;
	DRIVER driver = DRIVER::UNKNOWN;
	std::string name;
	std::string dongleFileName;
	std::string cdvdFileName;
	std::string hddFileName;
	std::string nandFileName;
	std::map<unsigned int, PS2::CControllerInfo::BUTTON> buttons;
	INPUT_MODE inputMode = INPUT_MODE::DEFAULT;
	std::array<float, 4> lightGunXform = {65535, 0, 65535, 0};
	uint32 eeFreqScaleNumerator = 1;
	uint32 eeFreqScaleDenominator = 1;
	std::string boot;
	std::vector<PATCH> patches;
};
