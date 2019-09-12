#include <cstring>
#include "../GsPixelFormats.h"
#include "../../Log.h"
#include "GSH_Vulkan.h"

#define LOG_NAME ("gsh_vulkan")

CGSH_Vulkan::CGSH_Vulkan()
{

}

CGSH_Vulkan::~CGSH_Vulkan()
{

}

void CGSH_Vulkan::InitializeImpl()
{
	assert(m_instance != nullptr);

	auto physicalDevices = GetPhysicalDevices();
	assert(!physicalDevices.empty());

	CreateDevice(physicalDevices[0]);

	VkSurfaceFormatKHR format{};
	format.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

	VkExtent2D extent{};
	extent.width = 640;
	extent.height = 480;
	CreateSwapChain(format, extent);
}

void CGSH_Vulkan::ReleaseImpl()
{
	ResetImpl();
}

void CGSH_Vulkan::ResetImpl()
{

}

void CGSH_Vulkan::FlipImpl()
{
	PresentBackbuffer();
	CGSHandler::FlipImpl();
}

void CGSH_Vulkan::LoadState(Framework::CZipArchiveReader& archive)
{
	CGSHandler::LoadState(archive);
}

void CGSH_Vulkan::NotifyPreferencesChangedImpl()
{
	CGSHandler::NotifyPreferencesChangedImpl();
}

std::vector<VkPhysicalDevice> CGSH_Vulkan::GetPhysicalDevices()
{
	auto result = VK_SUCCESS;

	uint32_t physicalDeviceCount = 0;
	result = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
	assert(result == VK_SUCCESS);
	
	CLog::GetInstance().Print(LOG_NAME, "Found %d physical devices.\r\n", physicalDeviceCount);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	result = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());
	assert(result == VK_SUCCESS);
	
	for(const auto& physicalDevice : physicalDevices)
	{
		CLog::GetInstance().Print(LOG_NAME, "Physical Device Info:\r\n");
		
		VkPhysicalDeviceProperties physicalDeviceProperties = {};
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		CLog::GetInstance().Print(LOG_NAME, "Driver Version: %d\r\n", physicalDeviceProperties.driverVersion);
		CLog::GetInstance().Print(LOG_NAME, "Device Name:    %s\r\n", physicalDeviceProperties.deviceName);
		CLog::GetInstance().Print(LOG_NAME, "Device Type:    %d\r\n", physicalDeviceProperties.deviceType);
		CLog::GetInstance().Print(LOG_NAME, "API Version:    %d.%d.%d\r\n",
			VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
			VK_VERSION_MINOR(physicalDeviceProperties.apiVersion),
			VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));
	}
	
	return physicalDevices;
}

void CGSH_Vulkan::CreateDevice(VkPhysicalDevice physicalDevice)
{
	assert(m_device == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	float queuePriorities[] = { 1.0f };
	
	auto deviceQueueCreateInfo = VkDeviceQueueCreateInfo{};
	deviceQueueCreateInfo.flags            = 0;
	deviceQueueCreateInfo.queueFamilyIndex = 0;
	deviceQueueCreateInfo.queueCount       = 1;
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

	std::vector<const char*> enabledExtensions;
	enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	
	std::vector<const char*> enabledLayers;
	
	auto deviceCreateInfo = VkDeviceCreateInfo{};
	deviceCreateInfo.flags                   = 0;
	deviceCreateInfo.enabledLayerCount       = enabledLayers.size();
	deviceCreateInfo.ppEnabledLayerNames     = enabledLayers.data();
	deviceCreateInfo.enabledExtensionCount   = enabledExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	deviceCreateInfo.pEnabledFeatures        = nullptr;
	deviceCreateInfo.queueCreateInfoCount    = 1;
	deviceCreateInfo.pQueueCreateInfos       = &deviceQueueCreateInfo;
	
	result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &m_device);
	assert(result == VK_SUCCESS);
}

void CGSH_Vulkan::CreateSwapChain(VkSurfaceFormatKHR surfaceFormat, VkExtent2D imageExtent)
{
	assert(m_device != VK_NULL_HANDLE);
	assert(m_swapChain == VK_NULL_HANDLE);
	assert(m_swapChainImages.empty());

	auto result = VK_SUCCESS;
	
	auto swapChainCreateInfo = VkSwapchainCreateInfoKHR{};
	swapChainCreateInfo.surface               = m_surface;
	swapChainCreateInfo.minImageCount         = 3; //Recommended by nVidia in UsingtheVulkanAPI_20160216.pdf
	swapChainCreateInfo.imageFormat           = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace       = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent           = imageExtent;
	swapChainCreateInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCreateInfo.imageArrayLayers      = 1;
	swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.queueFamilyIndexCount = 0;
	swapChainCreateInfo.pQueueFamilyIndices   = nullptr;
	swapChainCreateInfo.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
	swapChainCreateInfo.clipped               = VK_TRUE;
	swapChainCreateInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	result = vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapChain);
	assert(result == VK_SUCCESS);
	
	uint32_t imageCount = 0;
	result = vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
	assert(result == VK_SUCCESS);
	
	m_swapChainImages.resize(imageCount);
	result = vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
	assert(result == VK_SUCCESS);
}

/////////////////////////////////////////////////////////////
// Other Functions
/////////////////////////////////////////////////////////////

void CGSH_Vulkan::ProcessHostToLocalTransfer()
{

}

void CGSH_Vulkan::ProcessLocalToHostTransfer()
{

}

void CGSH_Vulkan::ProcessLocalToLocalTransfer()
{

}

void CGSH_Vulkan::ProcessClutTransfer(uint32 csa, uint32)
{

}

void CGSH_Vulkan::ReadFramebuffer(uint32 width, uint32 height, void* buffer)
{

}

Framework::CBitmap CGSH_Vulkan::GetScreenshot()
{
	return Framework::CBitmap();
}
