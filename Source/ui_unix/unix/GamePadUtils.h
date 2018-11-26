#pragma once

#include <libevdev.h>
#include <array>
#include "Types.h"

class CGamePadUtils
{
public:
	static std::array<uint32, 6> GetDeviceID(libevdev* dev);
	static bool ParseMAC(std::array<uint32, 6>&, std::string const&);
};
