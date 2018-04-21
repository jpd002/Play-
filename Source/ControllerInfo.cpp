#include "ControllerInfo.h"

using namespace PS2;

const char* CControllerInfo::m_buttonName[CControllerInfo::MAX_BUTTONS] =
    {
        "analog_left_x",
        "analog_left_y",
        "analog_right_x",
        "analog_right_y",
        "dpad_up",
        "dpad_down",
        "dpad_left",
        "dpad_right",
        "select",
        "start",
        "square",
        "triangle",
        "circle",
        "cross",
        "l1",
        "l2",
        "l3",
        "r1",
        "r2",
        "r3"};

bool CControllerInfo::IsAxis(BUTTON button)
{
	return (button == ANALOG_LEFT_X) ||
	       (button == ANALOG_LEFT_Y) ||
	       (button == ANALOG_RIGHT_X) ||
	       (button == ANALOG_RIGHT_Y);
}
