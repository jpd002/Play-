#include "GSH_VulkanDeviceInfo.h"
#include "GSH_Vulkan.h"
#include "string_format.h"

using namespace GSH_Vulkan;

CDeviceInfo::CDeviceInfo()
{
	PopulateDevices();
}

void CDeviceInfo::PopulateDevices()
{
	assert(m_log.empty());

	try
	{
		auto result = VK_SUCCESS;

		auto instance = CGSH_Vulkan::CreateInstance();

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

			auto propertyIterator = std::find_if(properties.begin(), properties.end(), [](const auto& property) { return (strcmp(property.extensionName, VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME) == 0); });
			if(propertyIterator == std::end(properties))
			{
				m_log += "Device not suitable: Doesn't support " VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME ".";
			}
			else
			{
				//Do something
			}

			m_log += "\r\n\r\n";
		}
	}
	catch(const std::exception& exception)
	{
		m_log += string_format("Got exception while scanning devices: %s.\r\n", exception.what());
	}
}

DeviceList CDeviceInfo::GetAvailableDevices() const
{
	return m_devices;
}

bool CDeviceInfo::HasAvailableDevices() const
{
	return !m_devices.empty();
}

std::string CDeviceInfo::GetLog() const
{
	return m_log;
}
