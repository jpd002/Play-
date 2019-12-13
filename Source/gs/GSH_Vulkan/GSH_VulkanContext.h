#pragma once

#include "vulkan/VulkanDef.h"
#include "vulkan/Device.h"
#include "vulkan/CommandBufferPool.h"
#include "../GSHandler.h"

namespace GSH_Vulkan
{
	class CContext
	{
	public:
		Framework::Vulkan::CInstance* instance = nullptr;
		VkPhysicalDevice physicalDevice;
		Framework::Vulkan::CDevice device;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSurfaceFormatKHR surfaceFormat;
		Framework::Vulkan::CCommandBufferPool commandBufferPool;
		VkQueue queue = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		VkImageView memoryImageView = VK_NULL_HANDLE;
		VkImageView clutImageView = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMCT32View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMT8View = VK_NULL_HANDLE;

		VkImageView GetSwizzleTable(uint32 psm) const
		{
			switch(psm)
			{
			default:
				assert(false);
			case CGSHandler::PSMCT32:
				return swizzleTablePSMCT32View;
			case CGSHandler::PSMT8:
				return swizzleTablePSMT8View;
			}
		}
	};

	typedef std::shared_ptr<CContext> ContextPtr;
}
