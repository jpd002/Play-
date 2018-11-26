#include "GamePadUtils.h"
#include <cstring>
#include <string>
#include <cstdio>

std::array<uint32, 6> CGamePadUtils::GetDeviceID(libevdev* dev)
{
	std::array<uint32, 6> device{0};
	if(libevdev_get_uniq(dev) != NULL)
	{
		if(!CGamePadUtils::ParseMAC(device, libevdev_get_uniq(dev)))
		{
			const char* tmp_id = libevdev_get_uniq(dev);
			if(strlen(tmp_id) >= 6)
			{
				for(int i = 0; i < strlen(tmp_id) && i < 6; ++i)
					device.at(i) = tmp_id[i];
			}
		}
	}
	if(device == std::array<uint32, 6>{0})
	{
		int vendor = libevdev_get_id_vendor(dev);
		int product = libevdev_get_id_product(dev);
		int ver = libevdev_get_id_version(dev);
		device.at(0) = vendor & 0xFF;
		device.at(1) = (vendor >> 8) & 0xFF;
		device.at(2) = product & 0xFF;
		device.at(3) = (product >> 8) & 0xFF;
		device.at(4) = ver & 0xFF;
		device.at(5) = (ver >> 8) & 0xFF;
	}
	return device;
}

bool CGamePadUtils::ParseMAC(std::array<uint32, 6>& out, std::string const& in)
{
	uint32 bytes[6] = {0};
	if(std::sscanf(in.c_str(),
	               "%02x:%02x:%02x:%02x:%02x:%02x",
	               &bytes[0], &bytes[1], &bytes[2],
	               &bytes[3], &bytes[4], &bytes[5]) != 6)
	{
		return false;
	}
	for(int i = 0; i < 6; ++i)
	{
		out.at(i) = bytes[i];
	}
	return true;
}
