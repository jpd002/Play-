#include "GSH_Vulkan.h"
#include <cstring>
#include "std_experimental_map.h"
#include "../GsPixelFormats.h"
#include "../GsTransferRange.h"
#include "Log.h"
#include "AppConfig.h"
#include "GSH_VulkanPlatformDefs.h"
#include "GSH_VulkanDrawDesktop.h"
#include "GSH_VulkanDrawMobile.h"
#include "GSH_VulkanDeviceInfo.h"
#include "vulkan/Loader.h"
#include "vulkan/StructDefs.h"
#include "vulkan/StructChain.h"
#include "vulkan/Utils.h"

#define LOG_NAME ("gsh_vulkan")

using namespace GSH_Vulkan;

static uint32 MakeColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	return (a << 24) | (b << 16) | (g << 8) | (r);
}

static uint16 RGBA32ToRGBA16(uint32 inputColor)
{
	uint32 result = 0;
	result |= ((inputColor & 0x000000F8) >> (0 + 3)) << 0;
	result |= ((inputColor & 0x0000F800) >> (8 + 3)) << 5;
	result |= ((inputColor & 0x00F80000) >> (16 + 3)) << 10;
	result |= ((inputColor & 0x80000000) >> 31) << 15;
	return result;
}

template <typename StorageFormat>
static Framework::Vulkan::CImage CreateSwizzleTable(Framework::Vulkan::CDevice& device,
                                                    const VkPhysicalDeviceMemoryProperties& memoryProperties, VkQueue queue, Framework::Vulkan::CCommandBufferPool& commandBufferPool)
{
	auto result = Framework::Vulkan::CImage(device, memoryProperties,
	                                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	                                        VK_FORMAT_R32_UINT, StorageFormat::PAGEWIDTH, StorageFormat::PAGEHEIGHT);
	result.Fill(queue, commandBufferPool, memoryProperties,
	            CGsPixelFormats::CPixelIndexor<StorageFormat>::GetPageOffsets());
	result.SetLayout(queue, commandBufferPool,
	                 VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT);
	return result;
}

CGSH_Vulkan::CGSH_Vulkan()
{
	m_context = std::make_shared<CContext>();
}

Framework::Vulkan::CInstance CGSH_Vulkan::CreateInstance(bool useValidationLayers)
{
	auto instanceCreateInfo = Framework::Vulkan::InstanceCreateInfo();

	std::vector<const char*> layers;
#if defined(_DEBUG)
	static const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	if(useValidationLayers)
	{
		useValidationLayers = Framework::Vulkan::CLoader::GetInstance().IsInstanceLayerPresent(validationLayerName);
	}
	if(useValidationLayers)
	{
		layers.push_back(validationLayerName);
	}
#endif

	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#if GSH_VULKAN_USE_ANNOTATIONS
	if(
	    Framework::Vulkan::CLoader::GetInstance().IsInstanceExtensionPresent(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
#if defined(__ANDROID__)
	    || useValidationLayers //On Android, the validation layer implements the debug utils extension, but it doesn't seem to be reported properly
#endif
	)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
#endif
#ifdef _WIN32
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef __APPLE__
#if TARGET_OS_IPHONE
	extensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#else
	extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif
#endif
#ifdef __linux__
#ifdef __ANDROID__
	extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#else
	extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#endif

	auto appInfo = Framework::Vulkan::ApplicationInfo();
	appInfo.pApplicationName = "Play!";
	appInfo.pEngineName = "Play!";
#if GSH_VULKAN_IS_DESKTOP
	appInfo.apiVersion = VK_API_VERSION_1_2;
#elif GSH_VULKAN_IS_MOBILE
	appInfo.apiVersion = VK_API_VERSION_1_0;
#else
#error Unsupported Vulkan flavor
#endif

	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	instanceCreateInfo.enabledLayerCount = static_cast<uint32>(layers.size());
	instanceCreateInfo.ppEnabledLayerNames = layers.data();
	return Framework::Vulkan::CInstance(instanceCreateInfo);
}

void CGSH_Vulkan::InitializeImpl()
{
	assert(!m_instance.IsEmpty());
	m_context->instance = &m_instance;

	auto physicalDevices = GetPhysicalDevices();
	assert(!physicalDevices.empty());
	auto physicalDeviceIndex = GetPhysicalDeviceIndex(physicalDevices);
	m_context->physicalDevice = physicalDevices[physicalDeviceIndex];

	auto renderQueueFamilies = GetRenderQueueFamilies(m_context->physicalDevice);
	assert(!renderQueueFamilies.empty());
	auto renderQueueFamily = renderQueueFamilies[0];

	m_instance.vkGetPhysicalDeviceMemoryProperties(m_context->physicalDevice, &m_context->physicalDeviceMemoryProperties);

	if(m_context->surface)
	{
		auto surfaceFormats = GetDeviceSurfaceFormats(m_context->physicalDevice);
		assert(surfaceFormats.size() > 0);
		int surfaceFormatIndex = 0;
		//Look for a suitable surface format
		for(int i = 0; i < surfaceFormats.size(); i++)
		{
			const auto& surfaceFormat = surfaceFormats[i];
			if(surfaceFormat.colorSpace != VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				continue;
			}
			if(
			    (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) ||
			    (surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM))
			{
				surfaceFormatIndex = i;
				break;
			}
		}
		m_context->surfaceFormat = surfaceFormats[surfaceFormatIndex];
	}

	CreateDevice(m_context->physicalDevice);
	m_context->device.vkGetDeviceQueue(m_context->device, renderQueueFamily, 0, &m_context->queue);
	m_context->commandBufferPool = Framework::Vulkan::CCommandBufferPool(m_context->device, renderQueueFamily);

	CreateDescriptorPool();
	CreateMemoryBuffer();
	CreateClutBuffer();

	m_swizzleTablePSMCT32 = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMCT32>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);
	m_swizzleTablePSMCT16 = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMCT16>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);
	m_swizzleTablePSMCT16S = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMCT16S>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);
	m_swizzleTablePSMT8 = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMT8>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);
	m_swizzleTablePSMT4 = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMT4>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);
	m_swizzleTablePSMZ32 = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMZ32>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);
	m_swizzleTablePSMZ16 = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMZ16>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);
	m_swizzleTablePSMZ16S = CreateSwizzleTable<CGsPixelFormats::STORAGEPSMZ16S>(m_context->device, m_context->physicalDeviceMemoryProperties, m_context->queue, m_context->commandBufferPool);

	m_context->swizzleTablePSMCT32View = m_swizzleTablePSMCT32.CreateImageView();
	m_context->swizzleTablePSMCT16View = m_swizzleTablePSMCT16.CreateImageView();
	m_context->swizzleTablePSMCT16SView = m_swizzleTablePSMCT16S.CreateImageView();
	m_context->swizzleTablePSMT8View = m_swizzleTablePSMT8.CreateImageView();
	m_context->swizzleTablePSMT4View = m_swizzleTablePSMT4.CreateImageView();
	m_context->swizzleTablePSMZ32View = m_swizzleTablePSMZ32.CreateImageView();
	m_context->swizzleTablePSMZ16View = m_swizzleTablePSMZ16.CreateImageView();
	m_context->swizzleTablePSMZ16SView = m_swizzleTablePSMZ16S.CreateImageView();

	m_context->annotations.SetImageName(m_swizzleTablePSMCT32, "Swizzle Table PSMCT32");
	m_context->annotations.SetImageName(m_swizzleTablePSMCT16, "Swizzle Table PSMCT16");
	m_context->annotations.SetImageName(m_swizzleTablePSMCT16S, "Swizzle Table PSMCT16S");
	m_context->annotations.SetImageName(m_swizzleTablePSMT8, "Swizzle Table PSMT8");
	m_context->annotations.SetImageName(m_swizzleTablePSMT4, "Swizzle Table PSMT4");
	m_context->annotations.SetImageName(m_swizzleTablePSMZ32, "Swizzle Table PSMZ32");
	m_context->annotations.SetImageName(m_swizzleTablePSMZ16, "Swizzle Table PSMZ16");
	m_context->annotations.SetImageName(m_swizzleTablePSMZ16S, "Swizzle Table PSMZ16S");

	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMCT32View, "Swizzle Table View PSMCT32");
	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMCT16View, "Swizzle Table View PSMCT16");
	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMCT16SView, "Swizzle Table View PSMCT16S");
	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMT8View, "Swizzle Table View PSMT8");
	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMT4View, "Swizzle Table View PSMT4");
	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMZ32View, "Swizzle Table View PSMZ32");
	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMZ16View, "Swizzle Table View PSMZ16");
	m_context->annotations.SetImageViewName(m_context->swizzleTablePSMZ16SView, "Swizzle Table View PSMZ16S");

	m_frameCommandBuffer = std::make_shared<CFrameCommandBuffer>(m_context);
	m_clutLoad = std::make_shared<CClutLoad>(m_context, m_frameCommandBuffer);
#if GSH_VULKAN_IS_DESKTOP
	m_draw = std::make_shared<CDrawDesktop>(m_context, m_frameCommandBuffer);
#elif GSH_VULKAN_IS_MOBILE
	m_draw = std::make_shared<CDrawMobile>(m_context, m_frameCommandBuffer);
#else
#error Unsupported Vulkan flavor
#endif
	if(m_context->surface)
	{
		m_present = std::make_shared<CPresent>(m_context);
	}
	m_transferHost = std::make_shared<CTransferHost>(m_context, m_frameCommandBuffer);
	m_transferLocal = std::make_shared<CTransferLocal>(m_context, m_frameCommandBuffer);

	m_frameCommandBuffer->RegisterWriter(m_draw.get());
	m_frameCommandBuffer->RegisterWriter(m_transferHost.get());
	m_frameCommandBuffer->BeginFrame();
}

void CGSH_Vulkan::ReleaseImpl()
{
	ResetImpl();

	//Flush any pending rendering commands
	m_context->device.vkQueueWaitIdle(m_context->queue);

	m_clutLoad.reset();
	m_draw.reset();
	m_present.reset();
	m_transferHost.reset();
	m_transferLocal.reset();
	m_frameCommandBuffer.reset();

	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMCT32View, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMCT16View, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMCT16SView, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMT8View, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMT4View, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMZ32View, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMZ16View, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_context->swizzleTablePSMZ16SView, nullptr);

	m_swizzleTablePSMCT32.Reset();
	m_swizzleTablePSMCT16.Reset();
	m_swizzleTablePSMCT16S.Reset();
	m_swizzleTablePSMT8.Reset();
	m_swizzleTablePSMT4.Reset();
	m_swizzleTablePSMZ32.Reset();
	m_swizzleTablePSMZ16.Reset();
	m_swizzleTablePSMZ16S.Reset();

	m_context->device.vkDestroyDescriptorPool(m_context->device, m_context->descriptorPool, nullptr);
	m_context->clutBuffer.Reset();
	m_context->memoryBuffer.Reset();
	m_context->memoryBufferCopy.Reset();
	m_context->memoryBufferTransfer.Reset();
	m_context->commandBufferPool.Reset();
	m_context->device.Reset();

	delete[] m_memoryCache;
	m_memoryCache = nullptr;
}

void CGSH_Vulkan::ResetImpl()
{
	m_vtxCount = 0;
	m_primitiveType = PRIM_INVALID;
	m_pendingPrim = false;
	m_pendingPrimValue = 0;
	m_regState.isValid = false;
	memset(&m_clutStates, 0, sizeof(m_clutStates));
	memset(m_memoryCache, 0, RAMSIZE);
	WriteBackMemoryCache();
}

void CGSH_Vulkan::SetPresentationParams(const CGSHandler::PRESENTATION_PARAMS& presentationParams)
{
	CGSHandler::SetPresentationParams(presentationParams);
	m_present->ValidateSwapChain(presentationParams);
}

void CGSH_Vulkan::MarkNewFrame()
{
	m_drawCallCount = m_frameCommandBuffer->GetFlushCount();
	m_frameCommandBuffer->ResetFlushCount();
	m_frameCommandBuffer->EndFrame();
	m_frameCommandBuffer->BeginFrame();
	CGSHandler::MarkNewFrame();
}

void CGSH_Vulkan::FlipImpl(const DISPLAY_INFO& dispInfo)
{
	if(m_present)
	{
		m_present->SetPresentationViewport(GetPresentationViewport());
		m_present->DoPresent(dispInfo);
	}

	PresentBackbuffer();
	for(auto& xferHistoryPair : m_xferHistory)
	{
		xferHistoryPair.second.Advance();
	}
	std::experimental::erase_if(m_xferHistory,
	                            [](const auto& xferTrackerPair) { return xferTrackerPair.second.IsEmpty(); });
	CGSHandler::FlipImpl(dispInfo);
}

std::vector<VkPhysicalDevice> CGSH_Vulkan::GetPhysicalDevices()
{
	auto result = VK_SUCCESS;

	uint32_t physicalDeviceCount = 0;
	result = m_instance.vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
	CHECKVULKANERROR(result);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	result = m_instance.vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());
	CHECKVULKANERROR(result);

	return physicalDevices;
}

uint32 CGSH_Vulkan::GetPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>& physicalDevices) const
{
	auto selectedDevice = CDeviceInfo::GetInstance().GetSelectedDevice();
	for(uint32 i = 0; i < physicalDevices.size(); i++)
	{
		const auto& physicalDevice = physicalDevices[i];
		VkPhysicalDeviceProperties physicalDeviceProperties = {};
		m_instance.vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		if(
		    (physicalDeviceProperties.deviceID == selectedDevice.deviceId) &&
		    (physicalDeviceProperties.vendorID == selectedDevice.vendorId))
		{
			return i;
		}
	}
	assert(false);
	return 0;
}

std::vector<uint32_t> CGSH_Vulkan::GetRenderQueueFamilies(VkPhysicalDevice physicalDevice)
{
	auto result = VK_SUCCESS;
	std::vector<uint32_t> renderQueueFamilies;

	uint32_t queueFamilyCount = 0;
	m_instance.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	CLog::GetInstance().Print(LOG_NAME, "Found %d queue families.\r\n", queueFamilyCount);

	std::vector<VkQueueFamilyProperties> queueFamilyPropertiesArray(queueFamilyCount);
	m_instance.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertiesArray.data());

	for(uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
	{
		bool graphicsSupported = false;

		CLog::GetInstance().Print(LOG_NAME, "Queue Family Info:\r\n");

		const auto& queueFamilyProperties = queueFamilyPropertiesArray[queueFamilyIndex];
		CLog::GetInstance().Print(LOG_NAME, "Queue Count:    %d\r\n", queueFamilyProperties.queueCount);
		CLog::GetInstance().Print(LOG_NAME, "Operating modes:\r\n");
		if(queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphicsSupported = true;
			CLog::GetInstance().Print(LOG_NAME, "  Graphics\r\n");
		}
		if(queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			CLog::GetInstance().Print(LOG_NAME, "  Compute\r\n");
		}
		if(queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			CLog::GetInstance().Print(LOG_NAME, "  Transfer\r\n");
		}
		if(queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
		{
			CLog::GetInstance().Print(LOG_NAME, "  Sparse Binding\r\n");
		}

		VkBool32 surfaceSupported = VK_FALSE;
		if(m_context->surface)
		{
			result = m_instance.vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_context->surface, &surfaceSupported);
			CHECKVULKANERROR(result);

			CLog::GetInstance().Print(LOG_NAME, "Supports surface: %d\r\n", surfaceSupported);
		}

		if(graphicsSupported && (!m_context->surface || surfaceSupported))
		{
			renderQueueFamilies.push_back(queueFamilyIndex);
		}
	}

	return renderQueueFamilies;
}

std::vector<VkSurfaceFormatKHR> CGSH_Vulkan::GetDeviceSurfaceFormats(VkPhysicalDevice physicalDevice)
{
	assert(m_context->surface != VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	uint32_t surfaceFormatCount = 0;
	result = m_instance.vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_context->surface, &surfaceFormatCount, nullptr);
	CHECKVULKANERROR(result);

	CLog::GetInstance().Print(LOG_NAME, "Found %d surface formats.\r\n", surfaceFormatCount);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	result = m_instance.vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_context->surface, &surfaceFormatCount, surfaceFormats.data());
	CHECKVULKANERROR(result);

	for(const auto& surfaceFormat : surfaceFormats)
	{
		CLog::GetInstance().Print(LOG_NAME, "Surface Format Info:\r\n");

		CLog::GetInstance().Print(LOG_NAME, "Format:      %d\r\n", surfaceFormat.format);
		CLog::GetInstance().Print(LOG_NAME, "Color Space: %d\r\n", surfaceFormat.colorSpace);
	}

	return surfaceFormats;
}

void CGSH_Vulkan::CreateDevice(VkPhysicalDevice physicalDevice)
{
	assert(m_context->device.IsEmpty());

	float queuePriorities[] = {1.0f};

	auto deviceQueueCreateInfo = Framework::Vulkan::DeviceQueueCreateInfo();
	deviceQueueCreateInfo.flags = 0;
	deviceQueueCreateInfo.queueFamilyIndex = 0;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

	std::vector<const char*> enabledExtensions;
	enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if GSH_VULKAN_IS_DESKTOP
	enabledExtensions.push_back(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);
#endif

	std::vector<const char*> enabledLayers;

	Framework::Vulkan::CStructChain createDeviceStructs;

#if GSH_VULKAN_IS_DESKTOP
	{
		auto physicalDeviceFeaturesInvocationInterlock = Framework::Vulkan::PhysicalDeviceFragmentShaderInterlockFeaturesEXT();
		physicalDeviceFeaturesInvocationInterlock.fragmentShaderPixelInterlock = VK_TRUE;
		createDeviceStructs.AddStruct(physicalDeviceFeaturesInvocationInterlock);
	}
#endif

	{
		auto physicalDeviceFeatures2 = Framework::Vulkan::PhysicalDeviceFeatures2KHR();
#ifndef __APPLE__
		//MoltenVK doesn't report this properly (probably due to mobile devices supporting buffer stores and not image stores)
		physicalDeviceFeatures2.features.fragmentStoresAndAtomics = VK_TRUE;
#endif
#if GSH_VULKAN_IS_DESKTOP
		physicalDeviceFeatures2.features.shaderInt16 = VK_TRUE;
#endif
		createDeviceStructs.AddStruct(physicalDeviceFeatures2);
	}

#if GSH_VULKAN_IS_DESKTOP
	{
		auto physicalDeviceVulkan12Features = Framework::Vulkan::PhysicalDeviceVulkan12Features();
		physicalDeviceVulkan12Features.shaderInt8 = VK_TRUE;
		physicalDeviceVulkan12Features.storageBuffer8BitAccess = VK_TRUE;
		physicalDeviceVulkan12Features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
		createDeviceStructs.AddStruct(physicalDeviceVulkan12Features);
	}

	{
		auto physicalDevice16BitStorageFeatures = Framework::Vulkan::PhysicalDevice16BitStorageFeatures();
		physicalDevice16BitStorageFeatures.storageBuffer16BitAccess = VK_TRUE;
		physicalDevice16BitStorageFeatures.uniformAndStorageBuffer16BitAccess = VK_TRUE;
		createDeviceStructs.AddStruct(physicalDevice16BitStorageFeatures);
	}
#endif

	auto deviceCreateInfo = Framework::Vulkan::DeviceCreateInfo();
	deviceCreateInfo.pNext = createDeviceStructs.GetNext();
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.enabledLayerCount = static_cast<uint32>(enabledLayers.size());
	deviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32>(enabledExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

	m_context->device = Framework::Vulkan::CDevice(m_instance, physicalDevice, deviceCreateInfo);

	{
		VkPhysicalDeviceProperties deviceProperties = {};
		m_context->instance->vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		m_context->storageBufferAlignment = deviceProperties.limits.minStorageBufferOffsetAlignment;
		m_context->computeWorkgroupInvocations = deviceProperties.limits.maxComputeWorkGroupInvocations;
	}

	m_context->annotations = decltype(m_context->annotations)(m_context->instance, &m_context->device);
}

void CGSH_Vulkan::CreateDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes;

	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = 0x800;
		poolSizes.push_back(poolSize);
	}

	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize.descriptorCount = 0x800;
		poolSizes.push_back(poolSize);
	}

	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		poolSize.descriptorCount = 0x800;
		poolSizes.push_back(poolSize);
	}

	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSize.descriptorCount = 0x800;
		poolSizes.push_back(poolSize);
	}

	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		poolSize.descriptorCount = 0x800;
		poolSizes.push_back(poolSize);
	}

	auto descriptorPoolCreateInfo = Framework::Vulkan::DescriptorPoolCreateInfo();
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = 0x1000;

	auto result = m_context->device.vkCreateDescriptorPool(m_context->device, &descriptorPoolCreateInfo, nullptr, &m_context->descriptorPool);
	CHECKVULKANERROR(result);
}

void CGSH_Vulkan::CreateMemoryBuffer()
{
	assert(m_context->memoryBuffer.IsEmpty());
	assert(!m_memoryCache);

	m_context->memoryBuffer = Framework::Vulkan::CBuffer(m_context->device,
	                                                     m_context->physicalDeviceMemoryProperties,
	                                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                                                     RAMSIZE);
	m_context->annotations.SetBufferName(m_context->memoryBuffer, "GS Memory");

	m_memoryCache = new uint8[RAMSIZE];
	memset(m_memoryCache, 0, RAMSIZE);

	m_context->memoryBuffer.Write(m_context->queue, m_context->commandBufferPool,
	                              m_context->physicalDeviceMemoryProperties, m_memoryCache);

	m_context->memoryBufferCopy = Framework::Vulkan::CBuffer(m_context->device,
	                                                         m_context->physicalDeviceMemoryProperties,
	                                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                                                         RAMSIZE);
	m_context->annotations.SetBufferName(m_context->memoryBufferCopy, "GS Memory Copy");

	m_context->memoryBufferTransfer = Framework::Vulkan::CBuffer(m_context->device,
	                                                             m_context->physicalDeviceMemoryProperties,
	                                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                                                             RAMSIZE);
	m_context->annotations.SetBufferName(m_context->memoryBufferTransfer, "GS Memory Transfer");
}

void CGSH_Vulkan::CreateClutBuffer()
{
	assert(m_context->clutBuffer.IsEmpty());

	static const uint32 clutBufferSize = CLUTENTRYCOUNT * sizeof(uint32) * CLUT_CACHE_SIZE;
	m_context->clutBuffer = Framework::Vulkan::CBuffer(m_context->device,
	                                                   m_context->physicalDeviceMemoryProperties,
	                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                                                   clutBufferSize);
	m_context->annotations.SetBufferName(m_context->clutBuffer, "CLUT Buffer");
}

void CGSH_Vulkan::ProcessPrim(uint64 data)
{
	unsigned int newPrimitiveType = static_cast<unsigned int>(data & 0x07);
	if(newPrimitiveType != m_primitiveType)
	{
		m_draw->FlushVertices();
	}
	m_primitiveType = newPrimitiveType;
	switch(m_primitiveType)
	{
	case PRIM_POINT:
		m_vtxCount = 1;
		break;
	case PRIM_LINE:
	case PRIM_LINESTRIP:
		m_vtxCount = 2;
		break;
	case PRIM_TRIANGLE:
	case PRIM_TRIANGLESTRIP:
	case PRIM_TRIANGLEFAN:
		m_vtxCount = 3;
		break;
	case PRIM_SPRITE:
		m_vtxCount = 2;
		break;
	}
}

void CGSH_Vulkan::VertexKick(uint8 registerId, uint64 data)
{
	if(m_pendingPrim)
	{
		m_pendingPrim = false;
		ProcessPrim(m_pendingPrimValue);
	}

	if(m_vtxCount == 0) return;

	bool drawingKick = (registerId == GS_REG_XYZ2) || (registerId == GS_REG_XYZF2);
	bool fog = (registerId == GS_REG_XYZF2) || (registerId == GS_REG_XYZF3);

	if(!m_drawEnabled) drawingKick = false;

	if(fog)
	{
		m_vtxBuffer[m_vtxCount - 1].position = data & 0x00FFFFFFFFFFFFFFULL;
		m_vtxBuffer[m_vtxCount - 1].rgbaq = m_nReg[GS_REG_RGBAQ];
		m_vtxBuffer[m_vtxCount - 1].uv = m_nReg[GS_REG_UV];
		m_vtxBuffer[m_vtxCount - 1].st = m_nReg[GS_REG_ST];
		m_vtxBuffer[m_vtxCount - 1].fog = static_cast<uint8>(data >> 56);
	}
	else
	{
		m_vtxBuffer[m_vtxCount - 1].position = data;
		m_vtxBuffer[m_vtxCount - 1].rgbaq = m_nReg[GS_REG_RGBAQ];
		m_vtxBuffer[m_vtxCount - 1].uv = m_nReg[GS_REG_UV];
		m_vtxBuffer[m_vtxCount - 1].st = m_nReg[GS_REG_ST];
		m_vtxBuffer[m_vtxCount - 1].fog = static_cast<uint8>(m_nReg[GS_REG_FOG] >> 56);
	}

	m_vtxCount--;

	if(m_vtxCount == 0)
	{
		if((m_nReg[GS_REG_PRMODECONT] & 1) != 0)
		{
			m_primitiveMode <<= m_nReg[GS_REG_PRIM];
		}
		else
		{
			m_primitiveMode <<= m_nReg[GS_REG_PRMODE];
		}

		if(drawingKick)
		{
			SetRenderingContext(m_primitiveMode);
		}

		switch(m_primitiveType)
		{
		case PRIM_POINT:
			if(drawingKick) Prim_Point();
			m_vtxCount = 1;
			break;
		case PRIM_LINE:
			if(drawingKick) Prim_Line();
			m_vtxCount = 2;
			break;
		case PRIM_LINESTRIP:
			if(drawingKick) Prim_Line();
			memcpy(&m_vtxBuffer[1], &m_vtxBuffer[0], sizeof(VERTEX));
			m_vtxCount = 1;
			break;
		case PRIM_TRIANGLE:
			if(drawingKick) Prim_Triangle();
			m_vtxCount = 3;
			break;
		case PRIM_TRIANGLESTRIP:
			if(drawingKick) Prim_Triangle();
			memcpy(&m_vtxBuffer[2], &m_vtxBuffer[1], sizeof(VERTEX));
			memcpy(&m_vtxBuffer[1], &m_vtxBuffer[0], sizeof(VERTEX));
			m_vtxCount = 1;
			break;
		case PRIM_TRIANGLEFAN:
			if(drawingKick) Prim_Triangle();
			memcpy(&m_vtxBuffer[1], &m_vtxBuffer[0], sizeof(VERTEX));
			m_vtxCount = 1;
			break;
		case PRIM_SPRITE:
			if(drawingKick) Prim_Sprite();
			m_vtxCount = 2;
			break;
		}
	}
}

static std::pair<uint32, uint32> GetMipLevelInfo(uint32 level, const CGSHandler::MIPTBP1& miptbp1, const CGSHandler::MIPTBP2& miptbp2)
{
	switch(level)
	{
	default:
		assert(false);
		return std::pair<uint32, uint32>(0, 0);
	case 1:
		return std::pair<uint32, uint32>(miptbp1.GetTbp1(), miptbp1.GetTbw1());
	case 2:
		return std::pair<uint32, uint32>(miptbp1.GetTbp2(), miptbp1.GetTbw2());
	case 3:
		return std::pair<uint32, uint32>(miptbp1.GetTbp3(), miptbp1.GetTbw3());
	case 4:
		return std::pair<uint32, uint32>(miptbp2.GetTbp4(), miptbp2.GetTbw4());
	case 5:
		return std::pair<uint32, uint32>(miptbp2.GetTbp5(), miptbp2.GetTbw5());
	case 6:
		return std::pair<uint32, uint32>(miptbp2.GetTbp6(), miptbp2.GetTbw6());
	}
}

void CGSH_Vulkan::SetRenderingContext(uint64 primReg)
{
	auto prim = make_convertible<PRMODE>(primReg);

	unsigned int context = prim.nContext;

	auto offset = make_convertible<XYOFFSET>(m_nReg[GS_REG_XYOFFSET_1 + context]);
	auto frame = make_convertible<FRAME>(m_nReg[GS_REG_FRAME_1 + context]);
	auto zbuf = make_convertible<ZBUF>(m_nReg[GS_REG_ZBUF_1 + context]);
	auto tex0 = make_convertible<TEX0>(m_nReg[GS_REG_TEX0_1 + context]);
	auto tex1 = make_convertible<TEX1>(m_nReg[GS_REG_TEX1_1 + context]);
	auto miptbp1 = make_convertible<MIPTBP1>(m_nReg[GS_REG_MIPTBP1_1 + context]);
	auto miptbp2 = make_convertible<MIPTBP2>(m_nReg[GS_REG_MIPTBP2_1 + context]);
	auto clamp = make_convertible<CLAMP>(m_nReg[GS_REG_CLAMP_1 + context]);
	auto alpha = make_convertible<ALPHA>(m_nReg[GS_REG_ALPHA_1 + context]);
	auto scissor = make_convertible<SCISSOR>(m_nReg[GS_REG_SCISSOR_1 + context]);
	auto test = make_convertible<TEST>(m_nReg[GS_REG_TEST_1 + context]);
	auto texA = make_convertible<TEXA>(m_nReg[GS_REG_TEXA]);
	auto fogCol = make_convertible<FOGCOL>(m_nReg[GS_REG_FOGCOL]);
	auto scanMask = m_nReg[GS_REG_SCANMSK] & 3;
	auto colClamp = m_nReg[GS_REG_COLCLAMP] & 1;
	auto fba = m_nReg[GS_REG_FBA_1 + context] & 1;

	auto pipelineCaps = make_convertible<CDraw::PIPELINE_CAPS>(0);
	if(prim.nTexture)
	{
		pipelineCaps.hasTexture = 1;
		pipelineCaps.textureHasAlpha = tex0.nColorComp;
		pipelineCaps.textureBlackIsTransparent = texA.nAEM;
		pipelineCaps.textureFunction = tex0.nFunction;
		pipelineCaps.texClampU = clamp.nWMS;
		pipelineCaps.texClampV = clamp.nWMT;
		pipelineCaps.textureFormat = tex0.nPsm;
		pipelineCaps.clutFormat = tex0.nCPSM;
	}
	pipelineCaps.hasFog = prim.nFog;
	pipelineCaps.scanMask = scanMask;
	pipelineCaps.hasAlphaBlending = prim.nAlpha && m_alphaBlendingEnabled;
	pipelineCaps.colClamp = colClamp;
	pipelineCaps.fba = fba;
	pipelineCaps.hasDstAlphaTest = test.nDestAlphaEnabled;
	pipelineCaps.dstAlphaTestRef = test.nDestAlphaMode;
	pipelineCaps.writeDepth = (zbuf.nMask == 0) && (test.nDepthEnabled != 0); //Depth test disabled -> no writes to depth buffer
	pipelineCaps.framebufferFormat = frame.nPsm;
	pipelineCaps.depthbufferFormat = zbuf.nPsm | 0x30;

	switch(m_primitiveType)
	{
	default:
		assert(false);
		[[fallthrough]];
	case PRIM_TRIANGLE:
	case PRIM_TRIANGLEFAN:
	case PRIM_TRIANGLESTRIP:
	case PRIM_SPRITE:
		pipelineCaps.primitiveType = CDraw::PIPELINE_PRIMITIVE_TRIANGLE;
		break;
	case PRIM_LINE:
	case PRIM_LINESTRIP:
		pipelineCaps.primitiveType = CDraw::PIPELINE_PRIMITIVE_LINE;
		break;
	case PRIM_POINT:
		pipelineCaps.primitiveType = CDraw::PIPELINE_PRIMITIVE_POINT;
		break;
	}

	uint32 fbWriteMask = ~frame.nMask;

	uint32 texBufPtr = tex0.GetBufPtr();
	uint32 texBufWidth = tex0.GetBufWidth();
	uint32 texMipLevel = 0;

	if(prim.nTexture)
	{
		bool minLinear = false;
		bool magLinear = false;

		switch(tex1.nMinFilter)
		{
		case MIN_FILTER_NEAREST:
		case MIN_FILTER_NEAREST_MIP_NEAREST:
		case MIN_FILTER_NEAREST_MIP_LINEAR:
			minLinear = false;
			break;
		case MIN_FILTER_LINEAR:
		case MIN_FILTER_LINEAR_MIP_NEAREST:
		case MIN_FILTER_LINEAR_MIP_LINEAR:
			minLinear = true;
			break;
		}

		switch(tex1.nMagFilter)
		{
		case MAG_FILTER_NEAREST:
			magLinear = false;
			break;
		case MAG_FILTER_LINEAR:
			magLinear = true;
			break;
		}

		//Ignore min filter if we don't have mipmap levels.
		if(tex1.nMaxMip == 0)
		{
			minLinear = magLinear;
		}
		else
		{
			bool hasMip = (tex1.nMinFilter >= MIN_FILTER_NEAREST_MIP_NEAREST);
			if(hasMip)
			{
				if(tex1.nLODMethod == LOD_CALC_STATIC)
				{
					int k = trunc(tex1.GetK());
					texMipLevel = std::clamp<int>(k, 0, tex1.nMaxMip);
					if(texMipLevel != 0)
					{
						auto mipLevelInfo = GetMipLevelInfo(texMipLevel, miptbp1, miptbp2);
						texBufPtr = mipLevelInfo.first;
						texBufWidth = mipLevelInfo.second;
					}
				}
				else
				{
					pipelineCaps.textureUseDynamicMipLOD = true;
				}
			}
		}

		pipelineCaps.textureUseLinearFiltering = (minLinear && magLinear);
	}

	if(prim.nAlpha)
	{
		pipelineCaps.alphaA = alpha.nA;
		pipelineCaps.alphaB = alpha.nB;
		pipelineCaps.alphaC = alpha.nC;
		pipelineCaps.alphaD = alpha.nD;
	}

	pipelineCaps.depthTestFunction = test.nDepthMethod;
	if(!test.nDepthEnabled || !m_depthTestingEnabled)
	{
		pipelineCaps.depthTestFunction = CGSHandler::DEPTH_TEST_ALWAYS;
	}

	//Disable depth writes if both frame buffer and z buffer share the same address
	//TODO: This could be refined to check if there's actually no frame buffer writes
	//(ie.: fbmsk, alpha testing)
	if(frame.GetBasePtr() == zbuf.GetBasePtr())
	{
		pipelineCaps.writeDepth = false;
	}

	pipelineCaps.alphaTestFunction = test.nAlphaMethod;
	pipelineCaps.alphaTestFailAction = test.nAlphaFail;
	if(!test.nAlphaEnabled || !m_alphaTestingEnabled)
	{
		pipelineCaps.alphaTestFunction = CGSHandler::ALPHA_TEST_ALWAYS;
	}

	//Convert alpha testing to write masking if possible
	if(
	    (pipelineCaps.alphaTestFunction == CGSHandler::ALPHA_TEST_NEVER) &&
	    (pipelineCaps.alphaTestFailAction == CGSHandler::ALPHA_TEST_FAIL_FBONLY))
	{
		pipelineCaps.alphaTestFunction = CGSHandler::ALPHA_TEST_ALWAYS;
		pipelineCaps.writeDepth = 0;
	}

	if(
	    (pipelineCaps.alphaTestFunction == CGSHandler::ALPHA_TEST_NEVER) &&
	    (pipelineCaps.alphaTestFailAction == CGSHandler::ALPHA_TEST_FAIL_RGBONLY))
	{
		pipelineCaps.alphaTestFunction = CGSHandler::ALPHA_TEST_ALWAYS;
		pipelineCaps.writeDepth = 0;
		fbWriteMask &= 0x00FFFFFF;
	}

	switch(frame.nPsm)
	{
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMZ32:
		pipelineCaps.maskColor = (fbWriteMask != 0xFFFFFFFF);
		break;
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMZ24:
		fbWriteMask = fbWriteMask & 0x00FFFFFF;
		pipelineCaps.maskColor = (fbWriteMask != 0x00FFFFFF);
		break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
	case CGSHandler::PSMZ16S:
		fbWriteMask = RGBA32ToRGBA16(fbWriteMask) & 0xFFFF;
		pipelineCaps.maskColor = (fbWriteMask != 0xFFFF);
		break;
	default:
		assert(false);
		break;
	}

	//Check if we need to save a copy of RAM because primitive writes to texture area
	bool needsTextureCopy = false;
	uint32 memoryCopyAddress = 0;
	uint32 memoryCopySize = 0;
	if((m_primitiveType == PRIM_SPRITE) && pipelineCaps.hasTexture)
	{
		uint32 texBufPtr = tex0.GetBufPtr();
		uint32 frameBufPtr = frame.GetBasePtr();
		uint32 depthBufPtr = zbuf.GetBasePtr();
		bool isTexUpperBytePsm = CGsPixelFormats::IsPsmUpperByte(tex0.nPsm);
		{
			bool isFrame24Bits = CGsPixelFormats::IsPsm24Bits(frame.nPsm);
			needsTextureCopy |= (texBufPtr == frameBufPtr) && !(isTexUpperBytePsm && isFrame24Bits);
		}
		if(pipelineCaps.writeDepth)
		{
			bool isDepth24Bits = CGsPixelFormats::IsPsm24Bits(zbuf.nPsm);
			needsTextureCopy |= (texBufPtr == depthBufPtr) && !(isTexUpperBytePsm && isDepth24Bits);
		}
		if(needsTextureCopy)
		{
			CGsCachedArea textureArea;
			textureArea.SetArea(tex0.nPsm, tex0.nBufPtr, tex0.GetBufWidth(), tex0.GetHeight());
			memoryCopyAddress = texBufPtr;
			memoryCopySize = textureArea.GetSize();
			if((memoryCopyAddress + memoryCopySize) > RAMSIZE)
			{
				//Some games (Castlevania: Curse of Darkness) have a texture that goes beyond
				//RAMSIZE, make sure we stay inside bounds.
				memoryCopySize = RAMSIZE - memoryCopyAddress;
			}
			m_draw->SetMemoryCopyParams(memoryCopyAddress, memoryCopySize);
		}
	}
	pipelineCaps.textureUseMemoryCopy = needsTextureCopy;

	m_draw->SetPipelineCaps(pipelineCaps);
	m_draw->SetFramebufferParams(frame.GetBasePtr(), frame.GetWidth(), fbWriteMask);
	m_draw->SetDepthbufferParams(zbuf.GetBasePtr(), frame.GetWidth());
	m_draw->SetTextureParams(texBufPtr, texBufWidth, tex0.GetWidth(), tex0.GetHeight(), texMipLevel, tex0.nCSA * 0x10);
	if(
	    pipelineCaps.textureUseDynamicMipLOD &&
	    (!m_regState.isValid ||
	     (m_regState.tex1 != tex1) ||
	     (m_regState.miptbp1 != miptbp1) ||
	     (m_regState.miptbp2 != miptbp2)))
	{
		CDraw::MipBufs mipBufs;
		for(int i = 1; i <= tex1.nMaxMip; i++)
		{
			mipBufs[i - 1] = GetMipLevelInfo(i, miptbp1, miptbp2);
		}
		for(int i = tex1.nMaxMip + 1; i <= 6; i++)
		{
			mipBufs[i - 1] = std::pair<uint32, uint32>(0, 0);
		}
		m_draw->SetMipParams(mipBufs, tex1.nMaxMip, tex1.GetK(), tex1.nLODL);
	}
	m_draw->SetTextureAlphaParams(texA.nTA0, texA.nTA1);
	m_draw->SetTextureClampParams(
	    clamp.GetMinU(), clamp.GetMinV(),
	    clamp.GetMaxU(), clamp.GetMaxV());
	m_draw->SetFogParams(
	    static_cast<float>(fogCol.nFCR) / 255.f,
	    static_cast<float>(fogCol.nFCG) / 255.f,
	    static_cast<float>(fogCol.nFCB) / 255.f);
	m_draw->SetAlphaBlendingParams(alpha.nFix);
	m_draw->SetAlphaTestParams(test.nAlphaRef);
	m_draw->SetScissor(scissor.scax0, scissor.scay0,
	                   scissor.scax1 - scissor.scax0 + 1,
	                   scissor.scay1 - scissor.scay0 + 1);

	m_fbBasePtr = frame.GetBasePtr();

	m_primOfsX = offset.GetX();
	m_primOfsY = offset.GetY();

	m_texWidth = tex0.GetWidth();
	m_texHeight = tex0.GetHeight();

	m_regState.isValid = true;
	m_regState.tex1 = tex1;
	m_regState.miptbp1 = miptbp1;
	m_regState.miptbp2 = miptbp2;
}

void CGSH_Vulkan::Prim_Point()
{
	auto xyz = make_convertible<XYZ>(m_vtxBuffer[0].position);
	auto rgbaq = make_convertible<RGBAQ>(m_vtxBuffer[0].rgbaq);

	float x = xyz.GetX();
	float y = xyz.GetY();
	uint32 z = xyz.nZ;

	x -= m_primOfsX;
	y -= m_primOfsY;

	auto color = MakeColor(
	    rgbaq.nR, rgbaq.nG,
	    rgbaq.nB, rgbaq.nA);

	// clang-format off
	CDraw::PRIM_VERTEX vertex =
	{
		//x, y, z, color, s, t, q, f
		  x, y, z, color, 0, 0, 1, 0,
	};
	// clang-format on

	m_draw->AddVertices(&vertex, &vertex + 1);
}

void CGSH_Vulkan::Prim_Line()
{
	XYZ pos[2];
	pos[0] <<= m_vtxBuffer[1].position;
	pos[1] <<= m_vtxBuffer[0].position;

	float x1 = pos[0].GetX(), x2 = pos[1].GetX();
	float y1 = pos[0].GetY(), y2 = pos[1].GetY();
	uint32 z1 = pos[0].nZ, z2 = pos[1].nZ;

	RGBAQ rgbaq[2];
	rgbaq[0] <<= m_vtxBuffer[1].rgbaq;
	rgbaq[1] <<= m_vtxBuffer[0].rgbaq;

	x1 -= m_primOfsX;
	x2 -= m_primOfsX;

	y1 -= m_primOfsY;
	y2 -= m_primOfsY;

	float s[2] = {0, 0};
	float t[2] = {0, 0};
	float q[2] = {1, 1};

	if(m_primitiveMode.nTexture)
	{
		if(m_primitiveMode.nUseUV)
		{
			UV uv[3];
			uv[0] <<= m_vtxBuffer[1].uv;
			uv[1] <<= m_vtxBuffer[0].uv;

			s[0] = uv[0].GetU() / static_cast<float>(m_texWidth);
			s[1] = uv[1].GetU() / static_cast<float>(m_texWidth);

			t[0] = uv[0].GetV() / static_cast<float>(m_texHeight);
			t[1] = uv[1].GetV() / static_cast<float>(m_texHeight);

			m_lastLineU = s[0];
			m_lastLineV = t[0];
		}
		else
		{
			ST st[2];
			st[0] <<= m_vtxBuffer[1].st;
			st[1] <<= m_vtxBuffer[0].st;

			s[0] = st[0].nS;
			s[1] = st[1].nS;
			t[0] = st[0].nT;
			t[1] = st[1].nT;

			q[0] = rgbaq[0].nQ;
			q[1] = rgbaq[1].nQ;
		}
	}

	auto color1 = MakeColor(
	    rgbaq[0].nR, rgbaq[0].nG,
	    rgbaq[0].nB, rgbaq[0].nA);

	auto color2 = MakeColor(
	    rgbaq[1].nR, rgbaq[1].nG,
	    rgbaq[1].nB, rgbaq[1].nA);

	// clang-format off
	CDraw::PRIM_VERTEX vertices[] =
	{
		{	x1, y1, z1, color1, s[0], t[0], q[0], 0 },
		{	x2, y2, z2, color2, s[1], t[1], q[1], 0 },
	};
	// clang-format on

	m_draw->AddVertices(std::begin(vertices), std::end(vertices));
}

void CGSH_Vulkan::Prim_Triangle()
{
	XYZ pos[3];
	pos[0] <<= m_vtxBuffer[2].position;
	pos[1] <<= m_vtxBuffer[1].position;
	pos[2] <<= m_vtxBuffer[0].position;

	float x1 = pos[0].GetX(), x2 = pos[1].GetX(), x3 = pos[2].GetX();
	float y1 = pos[0].GetY(), y2 = pos[1].GetY(), y3 = pos[2].GetY();
	uint32 z1 = pos[0].nZ, z2 = pos[1].nZ, z3 = pos[2].nZ;

	RGBAQ rgbaq[3];
	rgbaq[0] <<= m_vtxBuffer[2].rgbaq;
	rgbaq[1] <<= m_vtxBuffer[1].rgbaq;
	rgbaq[2] <<= m_vtxBuffer[0].rgbaq;

	x1 -= m_primOfsX;
	x2 -= m_primOfsX;
	x3 -= m_primOfsX;

	y1 -= m_primOfsY;
	y2 -= m_primOfsY;
	y3 -= m_primOfsY;

	float s[3] = {0, 0, 0};
	float t[3] = {0, 0, 0};
	float q[3] = {1, 1, 1};

	float f[3] = {0, 0, 0};

	if(m_primitiveMode.nFog)
	{
		f[0] = static_cast<float>(0xFF - m_vtxBuffer[2].fog) / 255.0f;
		f[1] = static_cast<float>(0xFF - m_vtxBuffer[1].fog) / 255.0f;
		f[2] = static_cast<float>(0xFF - m_vtxBuffer[0].fog) / 255.0f;
	}

	if(m_primitiveMode.nTexture)
	{
		if(m_primitiveMode.nUseUV)
		{
			UV uv[3];
			uv[0] <<= m_vtxBuffer[2].uv;
			uv[1] <<= m_vtxBuffer[1].uv;
			uv[2] <<= m_vtxBuffer[0].uv;

			s[0] = uv[0].GetU() / static_cast<float>(m_texWidth);
			s[1] = uv[1].GetU() / static_cast<float>(m_texWidth);
			s[2] = uv[2].GetU() / static_cast<float>(m_texWidth);

			t[0] = uv[0].GetV() / static_cast<float>(m_texHeight);
			t[1] = uv[1].GetV() / static_cast<float>(m_texHeight);
			t[2] = uv[2].GetV() / static_cast<float>(m_texHeight);
		}
		else
		{
			ST st[3];
			st[0] <<= m_vtxBuffer[2].st;
			st[1] <<= m_vtxBuffer[1].st;
			st[2] <<= m_vtxBuffer[0].st;

			s[0] = st[0].nS;
			s[1] = st[1].nS;
			s[2] = st[2].nS;
			t[0] = st[0].nT;
			t[1] = st[1].nT;
			t[2] = st[2].nT;

			q[0] = rgbaq[0].nQ;
			q[1] = rgbaq[1].nQ;
			q[2] = rgbaq[2].nQ;
		}
	}

	auto color1 = MakeColor(
	    rgbaq[0].nR, rgbaq[0].nG,
	    rgbaq[0].nB, rgbaq[0].nA);

	auto color2 = MakeColor(
	    rgbaq[1].nR, rgbaq[1].nG,
	    rgbaq[1].nB, rgbaq[1].nA);

	auto color3 = MakeColor(
	    rgbaq[2].nR, rgbaq[2].nG,
	    rgbaq[2].nB, rgbaq[2].nA);

	if(m_primitiveMode.nShading == 0)
	{
		//Flat shaded triangles use the last color set
		color1 = color2 = color3;
	}

	// clang-format off
	CDraw::PRIM_VERTEX vertices[] =
	{
		{	x1, y1, z1, color1, s[0], t[0], q[0], f[0]},
		{	x2, y2, z2, color2, s[1], t[1], q[1], f[1]},
		{	x3, y3, z3, color3, s[2], t[2], q[2], f[2]},
	};
	// clang-format on

	m_draw->AddVertices(std::begin(vertices), std::end(vertices));
}

void CGSH_Vulkan::Prim_Sprite()
{
	XYZ pos[2];
	pos[0] <<= m_vtxBuffer[1].position;
	pos[1] <<= m_vtxBuffer[0].position;

	float x1 = pos[0].GetX(), y1 = pos[0].GetY();
	float x2 = pos[1].GetX(), y2 = pos[1].GetY();
	uint32 z = pos[1].nZ;

	RGBAQ rgbaq[2];
	rgbaq[0] <<= m_vtxBuffer[1].rgbaq;
	rgbaq[1] <<= m_vtxBuffer[0].rgbaq;

	x1 -= m_primOfsX;
	x2 -= m_primOfsX;

	y1 -= m_primOfsY;
	y2 -= m_primOfsY;

	float s[2] = {0, 0};
	float t[2] = {0, 0};

	float f = 0;

	if(m_primitiveMode.nFog)
	{
		f = static_cast<float>(0xFF - m_vtxBuffer[0].fog) / 255.0f;
	}

	if(m_primitiveMode.nTexture)
	{
		if(m_primitiveMode.nUseUV)
		{
			UV uv[2];
			uv[0] <<= m_vtxBuffer[1].uv;
			uv[1] <<= m_vtxBuffer[0].uv;

			s[0] = uv[0].GetU() / static_cast<float>(m_texWidth);
			s[1] = uv[1].GetU() / static_cast<float>(m_texWidth);

			t[0] = uv[0].GetV() / static_cast<float>(m_texHeight);
			t[1] = uv[1].GetV() / static_cast<float>(m_texHeight);
		}
		else
		{
			ST st[2];

			st[0] <<= m_vtxBuffer[1].st;
			st[1] <<= m_vtxBuffer[0].st;

			float q1 = rgbaq[1].nQ;
			float q2 = rgbaq[0].nQ;
			if(q1 == 0) q1 = 1;
			if(q2 == 0) q2 = 1;

			s[0] = st[0].nS / q1;
			s[1] = st[1].nS / q2;

			t[0] = st[0].nT / q1;
			t[1] = st[1].nT / q2;
		}
	}

	auto color = MakeColor(
	    rgbaq[1].nR, rgbaq[1].nG,
	    rgbaq[1].nB, rgbaq[1].nA);

	// clang-format off
	CDraw::PRIM_VERTEX vertices[] =
	{
		{x1, y1, z, color, s[0], t[0], 1, f},
		{x2, y1, z, color, s[1], t[0], 1, f},
		{x1, y2, z, color, s[0], t[1], 1, f},

		{x1, y2, z, color, s[0], t[1], 1, f},
		{x2, y1, z, color, s[1], t[0], 1, f},
		{x2, y2, z, color, s[1], t[1], 1, f},
	};
	// clang-format on

	{
		const auto topLeftCorner = vertices;
		const auto bottomRightCorner = vertices + 5;
		CGsSpriteRect rect(topLeftCorner->x, topLeftCorner->y, bottomRightCorner->x, bottomRightCorner->y);
		CheckSpriteCachedClutInvalidation(rect);
	}

	m_draw->AddVertices(std::begin(vertices), std::end(vertices));
}

int32 CGSH_Vulkan::FindCachedClut(const CLUTKEY& key) const
{
	for(uint32 i = 0; i < CLUT_CACHE_SIZE; i++)
	{
		if(m_clutStates[i] == key)
		{
			return i;
		}
	}
	return -1;
}

void CGSH_Vulkan::CheckSpriteCachedClutInvalidation(const CGsSpriteRect& rect)
{
	//This is specific to Gran Turismo 4, but could be expanded to work with other games
	//GT4 writes to an area in GS RAM then uses this as a CLUT in one of its post-processing effects.
	//We flush the whole CLUT cache if we find a sprite draw that writes to any previous location of cached cluts
	if((rect.left == 0) && (rect.top == 0) && (rect.GetWidth() == 8) && (rect.GetHeight() == 16))
	{
		bool foundClut = false;
		for(uint32 i = 0; i < CLUT_CACHE_SIZE; i++)
		{
			uint32 cbpBasePtr = m_clutStates[i].cbp * 0x100;
			if(cbpBasePtr == m_fbBasePtr)
			{
				foundClut = true;
				break;
			}
		}
		if(foundClut)
		{
			memset(&m_clutStates, 0, sizeof(m_clutStates));
			m_draw->FlushVertices();
		}
	}
}

CGSH_Vulkan::CLUTKEY CGSH_Vulkan::MakeCachedClutKey(const TEX0& tex0, const TEXCLUT& texClut)
{
	auto clutKey = make_convertible<CLUTKEY>(0);
	clutKey.idx4 = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm);
	clutKey.cbp = tex0.nCBP;
	clutKey.cpsm = tex0.nCPSM;
	clutKey.csm = tex0.nCSM;
	clutKey.csa = tex0.nCSA;
	clutKey.cbw = texClut.nCBW;
	clutKey.cou = texClut.nCOU;
	clutKey.cov = texClut.nCOV;
	return clutKey;
}

/////////////////////////////////////////////////////////////
// Other Functions
/////////////////////////////////////////////////////////////

void CGSH_Vulkan::WriteRegisterImpl(uint8 registerId, uint64 data)
{
	//Some games such as Silent Hill 2 don't finish their transfers
	//completely: make sure we push the data to the GS's RAM nevertheless.
	if(!m_xferBuffer.empty() && (registerId != GS_REG_HWREG))
	{
		ProcessHostToLocalTransfer();
	}

	CGSHandler::WriteRegisterImpl(registerId, data);

	switch(registerId)
	{
	case GS_REG_PRIM:
		//Check if previous primitive set was a line
		if(m_primitiveType == PRIM_LINE)
		{
			//Virtua Fighter 2, Sega Rally 95 (and others):
			//Draws lines and uses result as CLUT for next draw calls.

			//Optimization: We check if the last drawn line UV matches what might currently be in the CLUT
			//If it's different, we force reset all saved CLUTs, otherwise, we assume we can keep
			//what we had previously. This helps reducing the number of compute dispatch needed to update the CLUT

			//Optimization lead: The game draws lines to update the CLUT area very often, but a lot of times
			//the result doesn't seem to be used. We could probably save a lot by not drawing these lines.

			if(
			    (m_clutLineU != m_lastLineU) ||
			    (m_clutLineV != m_lastLineV))
			{
				memset(&m_clutStates, 0, sizeof(m_clutStates));
			}
			m_clutLineU = m_lastLineU;
			m_clutLineV = m_lastLineV;
		}
		m_pendingPrim = true;
		m_pendingPrimValue = data;
		break;

	case GS_REG_XYZ2:
	case GS_REG_XYZ3:
	case GS_REG_XYZF2:
	case GS_REG_XYZF3:
		VertexKick(registerId, data);
		break;
	}
}

void CGSH_Vulkan::ProcessHostToLocalTransfer()
{
	//Flush previous cached info
	memset(&m_clutStates, 0, sizeof(m_clutStates));
	m_draw->FlushRenderPass();

	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);

	m_transferHost->Params.bufAddress = bltBuf.GetDstPtr();
	m_transferHost->Params.bufWidth = bltBuf.GetDstWidth();
	m_transferHost->Params.rrw = trxReg.nRRW;
	m_transferHost->Params.dsax = trxPos.nDSAX;
	m_transferHost->Params.dsay = trxPos.nDSAY;

	auto pipelineCaps = make_convertible<CTransferHost::PIPELINE_CAPS>(0);
	pipelineCaps.dstFormat = bltBuf.nDstPsm;

	m_transferHost->SetPipelineCaps(pipelineCaps);
	m_transferHost->DoTransfer(m_xferBuffer);

	m_xferBuffer.clear();
}

void CGSH_Vulkan::ProcessLocalToHostTransfer()
{
	bool readsEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSHANDLER_GS_RAM_READS_ENABLED);
	if(readsEnabled)
	{
		m_draw->FlushRenderPass();

		auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
		auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
		auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);

		m_xferHistory.insert(std::make_pair(bltBuf, LOCAL_TO_HOST_XFER_HISTORY{}));
		auto transfer = m_xferHistory.find(bltBuf);
		transfer->second.MarkUsed();

		auto [copyBase, copySize] = GsTransfer::GetSrcRange(bltBuf, trxReg, trxPos);

		//Some games do that, example: Star Ocean 3
		assert((copyBase + copySize) <= RAMSIZE);
		if(copyBase >= RAMSIZE)
		{
			//Huh, something really wierd happened
			return;
		}
		if((copyBase + copySize) > RAMSIZE)
		{
			copySize = RAMSIZE - copyBase;
		}

		auto& srcBuffer = m_context->memoryBuffer;
		auto& dstBuffer = m_context->memoryBufferTransfer;

		auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

		{
			auto memoryBarrier = Framework::Vulkan::MemoryBarrier();
			memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			                                       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
		}

		{
			VkBufferCopy bufferCopy = {};
			bufferCopy.size = copySize;
			bufferCopy.dstOffset = copyBase;
			bufferCopy.srcOffset = copyBase;
			m_context->device.vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);
		}

		if(!transfer->second.IsRecurring())
		{
			m_frameCommandBuffer->Flush();
			m_context->device.vkQueueWaitIdle(m_context->queue);
		}

		void* bufferPtr = nullptr;
		auto result = m_context->device.vkMapMemory(m_context->device, dstBuffer.GetMemory(), copyBase, copySize, 0, &bufferPtr);
		CHECKVULKANERROR(result);

		memcpy(m_memoryCache + copyBase, bufferPtr, copySize);

		m_context->device.vkUnmapMemory(m_context->device, dstBuffer.GetMemory());
	}
}

void CGSH_Vulkan::ProcessLocalToLocalTransfer()
{
	//Flush previous cached info
	memset(&m_clutStates, 0, sizeof(m_clutStates));
	m_draw->FlushRenderPass();

	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);

	assert(trxPos.nDIR == 0);

	m_transferLocal->Params.srcBufAddress = bltBuf.GetSrcPtr();
	m_transferLocal->Params.srcBufWidth = bltBuf.GetSrcWidth();
	m_transferLocal->Params.dstBufAddress = bltBuf.GetDstPtr();
	m_transferLocal->Params.dstBufWidth = bltBuf.GetDstWidth();
	m_transferLocal->Params.ssax = trxPos.nSSAX;
	m_transferLocal->Params.ssay = trxPos.nSSAY;
	m_transferLocal->Params.dsax = trxPos.nDSAX;
	m_transferLocal->Params.dsay = trxPos.nDSAY;
	m_transferLocal->Params.rrw = trxReg.nRRW;
	m_transferLocal->Params.rrh = trxReg.nRRH;

	auto pipelineCaps = make_convertible<CTransferLocal::PIPELINE_CAPS>(0);
	pipelineCaps.srcFormat = bltBuf.nSrcPsm;
	pipelineCaps.dstFormat = bltBuf.nDstPsm;

	if(bltBuf.GetSrcPtr() == bltBuf.GetDstPtr())
	{
		//If src and dst pointers are the same, we need to copy the memory area in our memoryBufferCopy.
		//This is needed for Tairyou Jigoku for a scrolling effect in the sewer level.
		//TODO: Handle overlapping regions (even if memory addresses don't match)

		auto [transferAddress, transferSize] = GsTransfer::GetSrcRange(bltBuf, trxReg, trxPos);

		//Since GetSrcRange returns values in page granularity, it's possible that we obtain an
		//[address, size] pair that goes beyond RAM's range. Happens in FF12's intro level.
		if((transferAddress + transferSize) > RAMSIZE)
		{
			transferSize = RAMSIZE - transferAddress;
		}

		auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = transferAddress;
		bufferCopy.dstOffset = transferAddress;
		bufferCopy.size = transferSize;

		m_context->device.vkCmdCopyBuffer(commandBuffer, m_context->memoryBuffer, m_context->memoryBufferCopy, 1, &bufferCopy);

		pipelineCaps.srcUseMemoryCopy = true;
	}

	m_transferLocal->SetPipelineCaps(pipelineCaps);
	m_transferLocal->DoTransfer();
}

void CGSH_Vulkan::ProcessClutTransfer(uint32 csa, uint32)
{
}

void CGSH_Vulkan::BeginTransferWrite()
{
	assert(m_xferBuffer.empty());
	m_xferBuffer.clear();
}

void CGSH_Vulkan::TransferWrite(const uint8* imageData, uint32 length)
{
	m_xferBuffer.insert(m_xferBuffer.end(), imageData, imageData + length);
}

void CGSH_Vulkan::WriteBackMemoryCache()
{
	m_frameCommandBuffer->Flush();
	m_context->device.vkQueueWaitIdle(m_context->queue);

	m_context->memoryBuffer.Write(m_context->queue, m_context->commandBufferPool,
	                              m_context->physicalDeviceMemoryProperties, m_memoryCache);
}

void CGSH_Vulkan::SyncMemoryCache()
{
	m_frameCommandBuffer->Flush();
	m_context->device.vkQueueWaitIdle(m_context->queue);

	m_context->memoryBuffer.Read(m_context->queue, m_context->commandBufferPool,
	                             m_context->physicalDeviceMemoryProperties, m_memoryCache);
}

void CGSH_Vulkan::SyncCLUT(const TEX0& tex0)
{
	if(!CGsPixelFormats::IsPsmIDTEX(tex0.nPsm)) return;
	if(!ProcessCLD(tex0)) return;

	auto texClut = make_convertible<TEXCLUT>(m_nReg[GS_REG_TEXCLUT]);

	auto clutKey = MakeCachedClutKey(tex0, texClut);
	int32 clutCacheIndex = FindCachedClut(clutKey);
	if(clutCacheIndex == -1)
	{
		clutCacheIndex = m_nextClutCacheIndex++;
		m_nextClutCacheIndex %= CLUT_CACHE_SIZE;
		m_clutStates[clutCacheIndex] = clutKey;

		m_draw->FlushRenderPass();
		uint32 clutBufferOffset = sizeof(uint32) * CLUTENTRYCOUNT * clutCacheIndex;
		m_clutLoad->DoClutLoad(clutBufferOffset, tex0, texClut);
	}

	uint32 clutBufferOffset = sizeof(uint32) * CLUTENTRYCOUNT * clutCacheIndex;
	m_draw->SetClutBufferOffset(clutBufferOffset);
}

uint8* CGSH_Vulkan::GetRam() const
{
	return m_memoryCache;
}

Framework::CBitmap CGSH_Vulkan::GetScreenshot()
{
	return Framework::CBitmap();
}

bool CGSH_Vulkan::GetDepthTestingEnabled() const
{
	return m_depthTestingEnabled;
}

void CGSH_Vulkan::SetDepthTestingEnabled(bool depthTestingEnabled)
{
	m_depthTestingEnabled = depthTestingEnabled;
}

bool CGSH_Vulkan::GetAlphaBlendingEnabled() const
{
	return m_alphaBlendingEnabled;
}

void CGSH_Vulkan::SetAlphaBlendingEnabled(bool alphaBlendingEnabled)
{
	m_alphaBlendingEnabled = alphaBlendingEnabled;
}

bool CGSH_Vulkan::GetAlphaTestingEnabled() const
{
	return m_alphaTestingEnabled;
}

void CGSH_Vulkan::SetAlphaTestingEnabled(bool alphaTestingEnabled)
{
	m_alphaTestingEnabled = alphaTestingEnabled;
}

Framework::CBitmap CGSH_Vulkan::GetFramebuffer(uint64 frameReg)
{
	Framework::CBitmap result;
	SendGSCall([&]() { result = GetFramebufferImpl(frameReg); }, true);
	return result;
}

Framework::CBitmap CGSH_Vulkan::GetDepthbuffer(uint64 frameReg, uint64 zbufReg)
{
	Framework::CBitmap result;
	SendGSCall([&]() { result = GetDepthbufferImpl(frameReg, zbufReg); }, true);
	return result;
}

Framework::CBitmap CGSH_Vulkan::GetTexture(uint64 tex0Reg, uint32 maxMip, uint64 miptbp1Reg, uint64 miptbp2Reg, uint32 mipLevel)
{
	Framework::CBitmap result;
	SendGSCall([&]() { result = GetTextureImpl(tex0Reg, maxMip, miptbp1Reg, miptbp2Reg, mipLevel); }, true);
	return result;
}

int CGSH_Vulkan::GetFramebufferScale()
{
	return 1;
}

const CGSHandler::VERTEX* CGSH_Vulkan::GetInputVertices() const
{
	return m_vtxBuffer;
}

Framework::CBitmap CGSH_Vulkan::GetFramebufferImpl(uint64 frameReg)
{
	auto frame = make_convertible<FRAME>(frameReg);

	SyncMemoryCache();

	uint32 frameWidth = frame.GetWidth();
	uint32 frameHeight = 1024;
	Framework::CBitmap bitmap;

	switch(frame.nPsm)
	{
	case PSMCT32:
	{
		bitmap = ReadImage32<CGsPixelFormats::CPixelIndexorPSMCT32>(GetRam(), frame.GetBasePtr(),
		                                                            frame.nWidth, frameWidth, frameHeight);
	}
	break;
	case PSMCT24:
	{
		bitmap = ReadImage32<CGsPixelFormats::CPixelIndexorPSMCT32, 0x00FFFFFF>(GetRam(), frame.GetBasePtr(),
		                                                                        frame.nWidth, frameWidth, frameHeight);
	}
	break;
	case PSMCT16:
	{
		bitmap = ReadImage16<CGsPixelFormats::CPixelIndexorPSMCT16>(GetRam(), frame.GetBasePtr(),
		                                                            frame.nWidth, frameWidth, frameHeight);
	}
	break;
	case PSMCT16S:
	{
		bitmap = ReadImage16<CGsPixelFormats::CPixelIndexorPSMCT16S>(GetRam(), frame.GetBasePtr(),
		                                                             frame.nWidth, frameWidth, frameHeight);
	}
	break;
	default:
		assert(false);
		break;
	}
	return bitmap;
}

Framework::CBitmap CGSH_Vulkan::GetDepthbufferImpl(uint64 frameReg, uint64 zbufReg)
{
	auto frame = make_convertible<FRAME>(frameReg);
	auto zbuf = make_convertible<ZBUF>(zbufReg);

	SyncMemoryCache();

	uint32 frameWidth = frame.GetWidth();
	uint32 frameHeight = 1024;
	Framework::CBitmap bitmap;

	switch(zbuf.nPsm | 0x30)
	{
	case PSMZ32:
	{
		bitmap = ReadImage32<CGsPixelFormats::CPixelIndexorPSMZ32>(GetRam(), zbuf.GetBasePtr(),
		                                                           frame.nWidth, frameWidth, frameHeight);
	}
	break;
	case PSMZ24:
	{
		bitmap = ReadImage32<CGsPixelFormats::CPixelIndexorPSMZ32, 0x00FFFFFF>(GetRam(), zbuf.GetBasePtr(),
		                                                                       frame.nWidth, frameWidth, frameHeight);
	}
	break;
	case PSMZ16S:
	{
		bitmap = ReadImage16<CGsPixelFormats::CPixelIndexorPSMZ16S>(GetRam(), zbuf.GetBasePtr(),
		                                                            frame.nWidth, frameWidth, frameHeight);
	}
	break;
	default:
		assert(false);
		break;
	}

	return bitmap;
}

Framework::CBitmap CGSH_Vulkan::GetTextureImpl(uint64 tex0Reg, uint32 maxMip, uint64 miptbp1Reg, uint64 miptbp2Reg, uint32 mipLevel)
{
	auto tex0 = make_convertible<TEX0>(tex0Reg);
	auto miptbp1 = make_convertible<MIPTBP1>(miptbp1Reg);
	auto miptbp2 = make_convertible<MIPTBP2>(miptbp2Reg);

	auto width = std::max<uint32>(tex0.GetWidth() >> mipLevel, 1);
	auto height = std::max<uint32>(tex0.GetHeight() >> mipLevel, 1);
	auto [tbp, tbw] =
	    [&]() {
		    switch(mipLevel)
		    {
		    default:
			    assert(false);
			    [[fallthrough]];
		    case 0:
			    return std::make_pair(tex0.GetBufPtr(), tex0.GetBufWidth());
		    case 1:
		    case 2:
		    case 3:
		    case 4:
		    case 5:
		    case 6:
			    return GetMipLevelInfo(mipLevel, miptbp1, miptbp2);
		    }
	    }();
	assert((tbw & 0x3F) == 0);
	tbw /= 64;

	SyncMemoryCache();

	Framework::CBitmap bitmap;

	switch(tex0.nPsm)
	{
	case PSMCT32:
		bitmap = ReadImage32<CGsPixelFormats::CPixelIndexorPSMCT32>(GetRam(), tbp, tbw, width, height);
		break;
	case PSMCT24:
		bitmap = ReadImage32<CGsPixelFormats::CPixelIndexorPSMCT32, 0x00FFFFFF>(GetRam(), tbp, tbw, width, height);
		break;
	case PSMCT16:
		bitmap = ReadImage16<CGsPixelFormats::CPixelIndexorPSMCT16>(GetRam(), tbp, tbw, width, height);
		break;
	case PSMCT16S:
		bitmap = ReadImage16<CGsPixelFormats::CPixelIndexorPSMCT16S>(GetRam(), tbp, tbw, width, height);
		break;
	case PSMT8:
		bitmap = ReadImage8<CGsPixelFormats::CPixelIndexorPSMT8>(GetRam(), tbp, tbw, width, height);
		break;
	case PSMT4:
		bitmap = ReadImage8<CGsPixelFormats::CPixelIndexorPSMT4>(GetRam(), tbp, tbw, width, height);
		break;
	}

	return bitmap;
}
