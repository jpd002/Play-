#include <cstring>
#include "../../AppConfig.h"
#include "GSH_VulkanDeviceInfo.h"
#include "GSH_Vulkan.h"
#include "string_format.h"

using namespace GSH_Vulkan;

CDeviceInfo::CDeviceInfo()
{
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CGSH_VULKAN_DEVICEID, 0);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CGSH_VULKAN_VENDORID, 0);

	PopulateDevices();

	m_selectedDevice.deviceId = CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSH_VULKAN_DEVICEID);
	m_selectedDevice.vendorId = CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSH_VULKAN_VENDORID);

	bool needsReset = (m_selectedDevice.deviceId == 0 || m_selectedDevice.vendorId == 0);
	needsReset |= !HasDevice(m_selectedDevice);

	if(needsReset && HasAvailableDevices())
	{
		SetSelectedDevice(m_devices[0]);
	}
}

void CDeviceInfo::PopulateDevices()
{
	assert(m_log.empty());

	try
	{
		auto result = VK_SUCCESS;

		//Create instance without validation layers to be as minimalist as possible
		auto instance = CGSH_Vulkan::CreateInstance(false);

		uint32_t physicalDeviceCount = 0;
		result = instance.vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		CHECKVULKANERROR(result);

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		result = instance.vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
		CHECKVULKANERROR(result);

		for(const auto& physicalDevice : physicalDevices)
		{
			VkPhysicalDeviceProperties physicalDeviceProperties = {};
			instance.vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			m_log += "------------------------------------------\r\n";
			m_log += "Physical Device Info:\r\n";
			m_log += string_format("Driver Version: %d\r\n", physicalDeviceProperties.driverVersion);
			m_log += string_format("Vendor ID:      %d\r\n", physicalDeviceProperties.vendorID);
			m_log += string_format("Device ID:      %d\r\n", physicalDeviceProperties.deviceID);
			m_log += string_format("Device Name:    %s\r\n", physicalDeviceProperties.deviceName);
			m_log += string_format("Device Type:    %d\r\n", physicalDeviceProperties.deviceType);
			m_log += string_format("API Version:    %d.%d.%d\r\n",
			                       VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
			                       VK_VERSION_MINOR(physicalDeviceProperties.apiVersion),
			                       VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));

			uint32_t propertyCount = 0;
			result = instance.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, nullptr);
			CHECKVULKANERROR(result);

			std::vector<VkExtensionProperties> properties(propertyCount);
			result = instance.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, properties.data());
			CHECKVULKANERROR(result);

			//auto propertyIterator = std::find_if(properties.begin(), properties.end(), [](const auto& property) { return (strcmp(property.extensionName, VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME) == 0); });
			//if(propertyIterator == std::end(properties))
			//{
			//	m_log += "Device not suitable: Doesn't support " VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME ".";
			//}
			//else
			{
				m_devices.push_back({physicalDeviceProperties.deviceName, physicalDeviceProperties.vendorID, physicalDeviceProperties.deviceID});
			}

			m_log += "\r\n\r\n";
		}
	}
	catch(const std::exception& exception)
	{
		m_log += string_format("Got exception while scanning devices: %s.\r\n", exception.what());
	}
}

bool CDeviceInfo::HasDevice(const VULKAN_DEVICE& deviceToTest) const
{
	for(const auto& device : m_devices)
	{
		if(
		    (device.deviceId == deviceToTest.deviceId) &&
		    (device.vendorId == deviceToTest.vendorId))
		{
			return true;
		}
	}
	return false;
}

DeviceList CDeviceInfo::GetAvailableDevices() const
{
	return m_devices;
}

VULKAN_DEVICE CDeviceInfo::GetSelectedDevice() const
{
	assert(HasDevice(m_selectedDevice));
	return m_selectedDevice;
}

void CDeviceInfo::SetSelectedDevice(const VULKAN_DEVICE& device)
{
	m_selectedDevice = device;
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSH_VULKAN_DEVICEID, m_selectedDevice.deviceId);
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSH_VULKAN_VENDORID, m_selectedDevice.vendorId);
}

bool CDeviceInfo::HasAvailableDevices() const
{
	return !m_devices.empty();
}

std::string CDeviceInfo::GetLog() const
{
	return m_log;
}
