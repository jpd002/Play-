#pragma once

#include <string>
#include <vector>
#include "ControllerInfo.h"

class CVirtualPad
{
public:
	struct ITEM
	{
		float                           x1 = 0;
		float                           y1 = 0;
		float                           x2 = 0;
		float                           y2 = 0;
		bool                            isAnalog = false;
		PS2::CControllerInfo::BUTTON    code0 = PS2::CControllerInfo::MAX_BUTTONS;
		PS2::CControllerInfo::BUTTON    code1 = PS2::CControllerInfo::MAX_BUTTONS;
		std::string                     imageName;
		std::string                     caption;
	};
	typedef std::vector<ITEM> ItemArray;

	static ItemArray    GetItems(float, float);

private:
	static ITEM    CreateButtonItem(float, float, float, float, PS2::CControllerInfo::BUTTON, const std::string&, const std::string& = "");
	static ITEM    CreateAnalogStickItem(float, float, float, float, PS2::CControllerInfo::BUTTON, PS2::CControllerInfo::BUTTON, const std::string&);
};
