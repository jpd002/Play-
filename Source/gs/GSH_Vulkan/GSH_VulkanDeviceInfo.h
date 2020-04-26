#pragma once

#include <vector>
#include <string>
#include "Singleton.h"
#include "Types.h"

namespace GSH_Vulkan
{
	struct VULKAN_DEVICE
	{
		uint32 vendorID = 0;
		uint32 deviceID = 0;
	};
	typedef std::vector<VULKAN_DEVICE> DeviceList;

	class CDeviceInfo : public CSingleton<CDeviceInfo>
	{
	public:
		CDeviceInfo();

		DeviceList GetAvailableDevices() const;
		bool HasAvailableDevices() const;
		std::string GetLog() const;

	private:
		void PopulateDevices();

		DeviceList m_devices;
		std::string m_log;
	};
}
