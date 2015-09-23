#pragma once

#include "Types.h"

namespace PS2
{
	class CControllerInfo
	{
	public:
		enum BUTTON
		{
			ANALOG_LEFT_X,
			ANALOG_LEFT_Y,
			ANALOG_RIGHT_X,
			ANALOG_RIGHT_Y,
			DPAD_UP,
			DPAD_DOWN,
			DPAD_LEFT,
			DPAD_RIGHT,
			SELECT,
			START,
			SQUARE,
			TRIANGLE,
			CIRCLE,
			CROSS,
			L1,
			L2,
			R1,
			R2,
			MAX_BUTTONS
		};

		static const char*	m_buttonName[MAX_BUTTONS];

		static bool			IsAxis(BUTTON);
	};
};
