#pragma once

#include "vulkan/VulkanDef.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Device.h"
#include "vulkan/Buffer.h"
#include "vulkan/CommandBufferPool.h"
#include "../GSHandler.h"

#ifdef _DEBUG
#define GSH_VULKAN_USE_ANNOTATIONS 1
#else
#define GSH_VULKAN_USE_ANNOTATIONS 0
#endif

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
		Framework::Vulkan::CBuffer memoryBufferTransfer;
		Framework::Vulkan::CBuffer clutBuffer;
		VkImageView swizzleTablePSMCT32View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMCT16View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMCT16SView = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMT8View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMT4View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMZ32View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMZ16View = VK_NULL_HANDLE;
		VkImageView swizzleTablePSMZ16SView = VK_NULL_HANDLE;

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
			case CGSHandler::PSMZ16:
				return swizzleTablePSMZ16View;
			case CGSHandler::PSMZ16S:
				return swizzleTablePSMZ16SView;
			}
		}

		void SetObjectName(uint64_t objectHandle, const char* objectName, VkObjectType objectType)
		{
#if GSH_VULKAN_USE_ANNOTATIONS
			auto objectNameInfo = Framework::Vulkan::DebugUtilsObjectNameInfoEXT();
			objectNameInfo.objectType = objectType;
			objectNameInfo.objectHandle = objectHandle;
			objectNameInfo.pObjectName = objectName;

			VkResult result = instance->vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo);
			CHECKVULKANERROR(result);
#endif
		}

		void PushCommandLabel(VkCommandBuffer commandBuffer, const char* labelName)
		{
#if GSH_VULKAN_USE_ANNOTATIONS
			auto labelInfo = Framework::Vulkan::DebugUtilsLabelEXT();
			labelInfo.pLabelName = labelName;
			labelInfo.color[0] = 1.0f;
			labelInfo.color[1] = 1.0f;
			labelInfo.color[2] = 1.0f;
			labelInfo.color[3] = 1.0f;
			instance->vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
#endif
		}

		void PopCommandLabel(VkCommandBuffer commandBuffer)
		{
#if GSH_VULKAN_USE_ANNOTATIONS
			instance->vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}

		void SetBufferName(VkBuffer buffer, const char* name)
		{
			SetObjectName((uint64_t)buffer, name, VK_OBJECT_TYPE_BUFFER);
		}

		void SetImageName(VkImage image, const char* name)
		{
			SetObjectName((uint64_t)image, name, VK_OBJECT_TYPE_IMAGE);
		}

		void SetImageViewName(VkImageView imageView, const char* name)
		{
			SetObjectName((uint64_t)imageView, name, VK_OBJECT_TYPE_IMAGE_VIEW);
		}
	};

	typedef std::shared_ptr<CContext> ContextPtr;
}
