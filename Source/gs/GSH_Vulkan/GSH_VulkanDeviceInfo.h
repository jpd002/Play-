#pragma once

#include <vector>
#include <string>
#include "Singleton.h"
#include "Types.h"

namespace GSH_Vulkan
{
	struct VULKAN_DEVICE
	{
		std::string deviceName;
		uint32 vendorId = 0;
		uint32 deviceId = 0;
	};
	typedef std::vector<VULKAN_DEVICE> DeviceList;

	class CDeviceInfo : public CSingleton<CDeviceInfo>
	{
	public:
		CDeviceInfo();

		DeviceList GetAvailableDevices() const;
		VULKAN_DEVICE GetSelectedDevice() const;
		bool HasAvailableDevices() const;
		std::string GetLog() const;

	private:
		void PopulateDevices();

		DeviceList m_devices;
		std::string m_log;
	};
}
