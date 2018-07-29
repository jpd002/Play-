#pragma once

#ifdef HAS_LIBEVDEV
#include <libevdev.h>
#endif
#include <array>
#include "Types.h"

class CGamePadUtils
{
public:
#ifdef HAS_LIBEVDEV
	static std::array<uint32, 6> GetDeviceID(libevdev* dev);
#endif
	static bool ParseMAC(std::array<uint32, 6>&, std::string const&);
};
