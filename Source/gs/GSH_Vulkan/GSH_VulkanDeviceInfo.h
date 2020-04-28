#pragma once

#include <vector>
#include <string>
#include "Singleton.h"
#include "Types.h"

#define PREF_CGSH_VULKAN_VENDORID "renderer.vulkan.vendorid"
#define PREF_CGSH_VULKAN_DEVICEID "renderer.vulkan.deviceid"

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
		bool HasAvailableDevices() const;
		std::string GetLog() const;

		VULKAN_DEVICE GetSelectedDevice() const;
		void SetSelectedDevice(const VULKAN_DEVICE&);

	private:
		void PopulateDevices();
		bool HasDevice(const VULKAN_DEVICE&) const;

		DeviceList m_devices;
		std::string m_log;
		VULKAN_DEVICE m_selectedDevice;
	};
}
