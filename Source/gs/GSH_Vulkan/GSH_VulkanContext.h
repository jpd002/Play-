#pragma once

#include "vulkan/VulkanDef.h"
#include "vulkan/Device.h"
#include "vulkan/CommandBufferPool.h"

namespace GSH_Vulkan
{
	class CContext
	{
	public:
		Framework::Vulkan::CDevice device;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSurfaceFormatKHR surfaceFormat;
		VkExtent2D surfaceExtents;
		Framework::Vulkan::CCommandBufferPool commandBufferPool;
		VkQueue queue = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	};

	typedef std::shared_ptr<CContext> ContextPtr;
}
