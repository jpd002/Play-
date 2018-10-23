#pragma once

#ifdef HAS_LIBEVDEV
#include <libevdev.h>
#elif defined(__APPLE__)
#include <IOKit/hid/IOHIDLib.h>
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <array>
#include "Types.h"

class CGamePadUtils
{
public:
#ifdef HAS_LIBEVDEV
	static std::array<uint32, 6> GetDeviceID(libevdev* dev);
#elif defined(__APPLE__)
	static int GetIntProperty(IOHIDDeviceRef device, CFStringRef key);
	static std::array<uint32, 6> GetDeviceID(IOHIDDeviceRef dev);
#endif
	static bool ParseMAC(std::array<uint32, 6>&, std::string const&);
};
