#pragma once

#include "vulkan/VulkanDef.h"
#include "vulkan/Device.h"
#include "vulkan/Buffer.h"
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
		VkSurfaceFormatKHR surfaceFormat = {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		uint32 storageBufferAlignment = 0;
		uint32 computeWorkgroupInvocations = 0;
		Framework::Vulkan::CCommandBufferPool commandBufferPool;
		VkQueue queue = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		Framework::Vulkan::CBuffer memoryBuffer;
		Framework::Vulkan::CBuffer memoryBufferCopy;
		Framework::Vulkan::CBuffer clutBuffer;
		VkImageView swizzleTablePSMCT32View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMCT16View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMCT16SView = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMT8View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMT4View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMZ32View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMZ16View = VK_NULL_HANDLE;

		VkImageView GetSwizzleTable(uint32 psm) const
		{
			switch(psm)
			{
			default:
				assert(false);
				[[fallthrough]];
			case CGSHandler::PSMCT32:
			case CGSHandler::PSMCT24:
			case CGSHandler::PSMCT24_UNK:
			case CGSHandler::PSMT8H:
			case CGSHandler::PSMT4HL:
			case CGSHandler::PSMT4HH:
				return swizzleTablePSMCT32View;
			case CGSHandler::PSMCT16:
				return swizzleTablePSMCT16View;
			case CGSHandler::PSMCT16S:
				return swizzleTablePSMCT16SView;
			case CGSHandler::PSMT8:
				return swizzleTablePSMT8View;
			case CGSHandler::PSMT4:
				return swizzleTablePSMT4View;
			case CGSHandler::PSMZ32:
			case CGSHandler::PSMZ24:
				return swizzleTablePSMZ32View;
				break;
			case CGSHandler::PSMZ16:
				return swizzleTablePSMZ16View;
				break;
			case CGSHandler::PSMZ16S:
				//TODO: Use real swizzle table
				return swizzleTablePSMCT16SView;
				break;
			}
		}
	};

	typedef std::shared_ptr<CContext> ContextPtr;
}
