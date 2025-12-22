#pragma once

#include <vector>
#include <string>
#include <map>
#include "Types.h"
#include "ControllerInfo.h"

struct ARCADE_MACHINE_DEF
{
	enum DRIVER
	{
		UNKNOWN,
		NAMCO_SYSTEM_246,
		NAMCO_SYSTEM_147,
	};

	enum class INPUT_MODE
	{
		DEFAULT,
		LIGHTGUN,
		DRUM,
		DRIVE,
		TOUCH,
	};

	enum class IO_MODE
	{
		DEFAULT,
		SYS147_AI,
	};

	struct PATCH
	{
		uint32 address = 0;
		uint32 value = 0;
	};

	using ButtonSelector = std::pair<int, PS2::CControllerInfo::BUTTON>;

	std::string id;
	std::string parent;
	DRIVER driver = DRIVER::UNKNOWN;
	std::string name;
	std::string dongleFileName;
	std::string cdvdFileName;
	std::string hddFileName;
	std::string nandFileName;
	std::map<std::string, uint32> nandMounts;
	std::map<unsigned int, ButtonSelector> buttons;
	INPUT_MODE inputMode = INPUT_MODE::DEFAULT;
	IO_MODE ioMode = IO_MODE::DEFAULT;
	std::string boardId;
	std::array<float, 4> screenPosXform = {65535, 0, 65535, 0};
	uint32 eeFreqScaleNumerator = 1;
	uint32 eeFreqScaleDenominator = 1;
	std::string boot;
	std::vector<PATCH> patches;
};
