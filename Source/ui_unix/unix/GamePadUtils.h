#pragma once

#include <libevdev.h>
#include <array>
#include "Types.h"

typedef std::array<uint32, 6> GamePadDeviceId;

class CGamePadUtils
{
public:
	static GamePadDeviceId GetDeviceID(libevdev* dev);
	static bool ParseMAC(GamePadDeviceId&, std::string const&);
};
