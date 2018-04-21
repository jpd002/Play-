#pragma once

#include "Types.h"
#include <array>
#include <libevdev.h>

class CGamePadUtils
{
public:
	static std::array<uint32, 6> GetDeviceID(libevdev* dev);
	static bool ParseMAC(std::array<uint32, 6>&, std::string const&);
};
