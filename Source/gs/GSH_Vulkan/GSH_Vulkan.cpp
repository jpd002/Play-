#include <cstring>
#include "../GsPixelFormats.h"
#include "../../Log.h"
#include "GSH_Vulkan.h"
#include "GSH_VulkanDeviceInfo.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"

#define LOG_NAME ("gsh_vulkan")

using namespace GSH_Vulkan;

//#define FILL_IMAGES

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

	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef __APPLE__
	extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif
#ifdef __linux__
	extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

	std::vector<const char*> layers;
#if defined(_DEBUG) && !defined(__APPLE__)
	if(useValidationLayers)
	{
		layers.push_back("VK_LAYER_LUNARG_standard_validation");
	}
#endif

	auto appInfo = Framework::Vulkan::ApplicationInfo();
	appInfo.pApplicationName = "Play!";
	appInfo.pEngineName = "Play!";
#ifdef __APPLE__
	//MoltenVK requires version to be 1.0.x
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
#else
	appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
#endif

	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	instanceCreateInfo.enabledLayerCount = layers.size();
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

	auto surfaceFormats = GetDeviceSurfaceFormats(m_context->physicalDevice);
	assert(surfaceFormats.size() > 0);
	m_context->surfaceFormat = surfaceFormats[0];

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

	m_context->swizzleTablePSMCT32View = m_swizzleTablePSMCT32.CreateImageView();
	m_context->swizzleTablePSMCT16View = m_swizzleTablePSMCT16.CreateImageView();
	m_context->swizzleTablePSMCT16SView = m_swizzleTablePSMCT16S.CreateImageView();
	m_context->swizzleTablePSMT8View = m_swizzleTablePSMT8.CreateImageView();
	m_context->swizzleTablePSMT4View = m_swizzleTablePSMT4.CreateImageView();
	m_context->swizzleTablePSMZ32View = m_swizzleTablePSMZ32.CreateImageView();
	m_context->swizzleTablePSMZ16View = m_swizzleTablePSMZ16.CreateImageView();

	m_frameCommandBuffer = std::make_shared<CFrameCommandBuffer>(m_context);
	m_clutLoad = std::make_shared<CClutLoad>(m_context, m_frameCommandBuffer);
	m_draw = std::make_shared<CDraw>(m_context, m_frameCommandBuffer);
	m_present = std::make_shared<CPresent>(m_context);
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

	m_swizzleTablePSMCT32.Reset();
	m_swizzleTablePSMCT16.Reset();
	m_swizzleTablePSMCT16S.Reset();
	m_swizzleTablePSMT8.Reset();
	m_swizzleTablePSMT4.Reset();
	m_swizzleTablePSMZ32.Reset();
	m_swizzleTablePSMZ16.Reset();

	m_context->device.vkDestroyDescriptorPool(m_context->device, m_context->descriptorPool, nullptr);
	m_context->clutBuffer.Reset();
	m_context->device.vkUnmapMemory(m_context->device, m_context->memoryBuffer.GetMemory());
	m_context->memoryBuffer.Reset();
	m_context->commandBufferPool.Reset();
	m_context->device.Reset();
}

void CGSH_Vulkan::ResetImpl()
{
	m_vtxCount = 0;
	m_primitiveType = PRIM_INVALID;
	memset(&m_clutStates, 0, sizeof(m_clutStates));
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
	CGSHandler::MarkNewFrame();
	m_frameCommandBuffer->BeginFrame();
}

void CGSH_Vulkan::FlipImpl()
{
	auto dispInfo = GetCurrentDisplayInfo();
	auto fb = make_convertible<DISPFB>(dispInfo.first);
	auto d = make_convertible<DISPLAY>(dispInfo.second);

	unsigned int dispWidth = (d.nW + 1) / (d.nMagX + 1);
	unsigned int dispHeight = (d.nH + 1);

	bool halfHeight = GetCrtIsInterlaced() && GetCrtIsFrameMode();
	if(halfHeight) dispHeight /= 2;

	m_present->SetPresentationViewport(GetPresentationViewport());
	m_present->DoPresent(fb.nPSM, fb.GetBufPtr(), fb.GetBufWidth(), dispWidth, dispHeight);

	PresentBackbuffer();
	CGSHandler::FlipImpl();
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
	assert(m_context->surface != VK_NULL_HANDLE);

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
		result = m_instance.vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_context->surface, &surfaceSupported);
		CHECKVULKANERROR(result);

		CLog::GetInstance().Print(LOG_NAME, "Supports surface: %d\r\n", surfaceSupported);

		if(graphicsSupported && surfaceSupported)
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
	enabledExtensions.push_back(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);

	std::vector<const char*> enabledLayers;

	auto physicalDeviceFeaturesInvocationInterlock = Framework::Vulkan::PhysicalDeviceFragmentShaderInterlockFeaturesEXT();
	physicalDeviceFeaturesInvocationInterlock.fragmentShaderPixelInterlock = VK_TRUE;

	auto physicalDeviceFeatures2 = Framework::Vulkan::PhysicalDeviceFeatures2KHR();
	physicalDeviceFeatures2.pNext = &physicalDeviceFeaturesInvocationInterlock;
	physicalDeviceFeatures2.features.fragmentStoresAndAtomics = VK_TRUE;

	auto deviceCreateInfo = Framework::Vulkan::DeviceCreateInfo();
	deviceCreateInfo.pNext = &physicalDeviceFeatures2;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.enabledLayerCount = static_cast<uint32>(enabledLayers.size());
	deviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32>(enabledExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

	m_context->device = Framework::Vulkan::CDevice(m_instance, physicalDevice, deviceCreateInfo);
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
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

	m_context->memoryBuffer = Framework::Vulkan::CBuffer(m_context->device,
	                                                     m_context->physicalDeviceMemoryProperties, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, RAMSIZE);

#ifdef FILL_IMAGES
	{
		uint32* memory = nullptr;
		m_context->device.vkMapMemory(m_context->device, m_context->memoryBuffer.GetMemory(),
		                              0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&memory));
		memset(memory, 0x80, RAMSIZE);
		m_context->device.vkUnmapMemory(m_context->device, m_context->memoryBuffer.GetMemory());
	}
#endif

	auto result = m_context->device.vkMapMemory(m_context->device, m_context->memoryBuffer.GetMemory(),
	                                            0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&m_memoryBufferPtr));
	CHECKVULKANERROR(result);
}

void CGSH_Vulkan::CreateClutBuffer()
{
	assert(m_context->clutBuffer.IsEmpty());

	static const uint32 clutBufferSize = CLUTENTRYCOUNT * sizeof(uint32) * CLUT_CACHE_SIZE;
	m_context->clutBuffer = Framework::Vulkan::CBuffer(m_context->device,
	                                                   m_context->physicalDeviceMemoryProperties, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, clutBufferSize);

#ifdef FILL_IMAGES
	{
		uint32* memory = nullptr;
		m_context->device.vkMapMemory(m_context->device, m_context->clutBuffer.GetMemory(),
		                              0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&memory));
		memset(memory, 0x80, clutBufferSize);
		m_context->device.vkUnmapMemory(m_context->device, m_context->clutBuffer.GetMemory());
	}
#endif
}

void CGSH_Vulkan::VertexKick(uint8 registerId, uint64 data)
{
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
#if 0
		case PRIM_POINT:
			if(nDrawingKick) Prim_Point();
			m_nVtxCount = 1;
			break;
		case PRIM_LINE:
			if(nDrawingKick) Prim_Line();
			m_nVtxCount = 2;
			break;
		case PRIM_LINESTRIP:
			if(nDrawingKick) Prim_Line();
			memcpy(&m_VtxBuffer[1], &m_VtxBuffer[0], sizeof(VERTEX));
			m_nVtxCount = 1;
			break;
#endif
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

void CGSH_Vulkan::SetRenderingContext(uint64 primReg)
{
	auto prim = make_convertible<PRMODE>(primReg);

	unsigned int context = prim.nContext;

	auto offset = make_convertible<XYOFFSET>(m_nReg[GS_REG_XYOFFSET_1 + context]);
	auto frame = make_convertible<FRAME>(m_nReg[GS_REG_FRAME_1 + context]);
	auto zbuf = make_convertible<ZBUF>(m_nReg[GS_REG_ZBUF_1 + context]);
	auto tex0 = make_convertible<TEX0>(m_nReg[GS_REG_TEX0_1 + context]);
	auto clamp = make_convertible<CLAMP>(m_nReg[GS_REG_CLAMP_1 + context]);
	auto alpha = make_convertible<ALPHA>(m_nReg[GS_REG_ALPHA_1 + context]);
	auto scissor = make_convertible<SCISSOR>(m_nReg[GS_REG_SCISSOR_1 + context]);
	auto test = make_convertible<TEST>(m_nReg[GS_REG_TEST_1 + context]);
	auto texA = make_convertible<TEXA>(m_nReg[GS_REG_TEXA]);
	auto fogCol = make_convertible<FOGCOL>(m_nReg[GS_REG_FOGCOL]);

	auto pipelineCaps = make_convertible<CDraw::PIPELINE_CAPS>(0);
	pipelineCaps.hasTexture = prim.nTexture;
	pipelineCaps.textureHasAlpha = tex0.nColorComp;
	pipelineCaps.textureBlackIsTransparent = texA.nAEM;
	pipelineCaps.textureFunction = tex0.nFunction;
	pipelineCaps.texClampU = clamp.nWMS;
	pipelineCaps.texClampV = clamp.nWMT;
	pipelineCaps.hasFog = prim.nFog;
	pipelineCaps.hasAlphaBlending = prim.nAlpha;
	pipelineCaps.hasDstAlphaTest = test.nDestAlphaEnabled;
	pipelineCaps.dstAlphaTestRef = test.nDestAlphaMode;
	pipelineCaps.writeDepth = (zbuf.nMask == 0);
	pipelineCaps.textureFormat = tex0.nPsm;
	pipelineCaps.clutFormat = tex0.nCPSM;
	pipelineCaps.framebufferFormat = frame.nPsm;
	pipelineCaps.depthbufferFormat = zbuf.nPsm | 0x30;

	uint32 fbWriteMask = ~frame.nMask;

	if(prim.nAlpha)
	{
		pipelineCaps.alphaA = alpha.nA;
		pipelineCaps.alphaB = alpha.nB;
		pipelineCaps.alphaC = alpha.nC;
		pipelineCaps.alphaD = alpha.nD;
	}

	pipelineCaps.depthTestFunction = test.nDepthMethod;
	if(!test.nDepthEnabled)
	{
		pipelineCaps.depthTestFunction = CGSHandler::DEPTH_TEST_ALWAYS;
	}

	pipelineCaps.alphaTestFunction = test.nAlphaMethod;
	pipelineCaps.alphaTestFailAction = test.nAlphaFail;
	if(!test.nAlphaEnabled)
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
		pipelineCaps.maskColor = (fbWriteMask != 0xFFFFFFFF);
		break;
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMZ24:
		fbWriteMask = fbWriteMask & 0x00FFFFFF;
		pipelineCaps.maskColor = (fbWriteMask != 0x00FFFFFF);
		break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
		fbWriteMask = RGBA32ToRGBA16(fbWriteMask) & 0xFFFF;
		pipelineCaps.maskColor = (fbWriteMask != 0xFFFF);
		break;
	default:
		assert(false);
		break;
	}

	m_draw->SetPipelineCaps(pipelineCaps);
	m_draw->SetFramebufferParams(frame.GetBasePtr(), frame.GetWidth(), fbWriteMask);
	m_draw->SetDepthbufferParams(zbuf.GetBasePtr(), frame.GetWidth());
	m_draw->SetTextureParams(tex0.GetBufPtr(), tex0.GetBufWidth(),
	                         tex0.GetWidth(), tex0.GetHeight(), tex0.nCSA * 0x10);
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

	m_primOfsX = offset.GetX();
	m_primOfsY = offset.GetY();

	m_texWidth = tex0.GetWidth();
	m_texHeight = tex0.GetHeight();
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
		{x1, y1, z, color, s[0], t[0], 1, 0},
		{x2, y1, z, color, s[1], t[0], 1, 0},
		{x1, y2, z, color, s[0], t[1], 1, 0},

		{x1, y2, z, color, s[0], t[1], 1, 0},
		{x2, y1, z, color, s[1], t[0], 1, 0},
		{x2, y2, z, color, s[1], t[1], 1, 0},
	};
	// clang-format on

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
	CGSHandler::WriteRegisterImpl(registerId, data);

	switch(registerId)
	{
	case GS_REG_PRIM:
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
}

void CGSH_Vulkan::ProcessLocalToHostTransfer()
{
	//We're about to read from GS RAM, make sure all rendering commands are complete
	m_frameCommandBuffer->Flush();
	m_context->device.vkQueueWaitIdle(m_context->queue);
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

	m_transferLocal->SetPipelineCaps(pipelineCaps);
	m_transferLocal->DoTransfer();
}

void CGSH_Vulkan::ProcessClutTransfer(uint32 csa, uint32)
{
}

void CGSH_Vulkan::BeginTransferWrite()
{
	m_xferBuffer.clear();
}

void CGSH_Vulkan::TransferWrite(const uint8* imageData, uint32 length)
{
	m_xferBuffer.insert(m_xferBuffer.end(), imageData, imageData + length);
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

void CGSH_Vulkan::ReadFramebuffer(uint32 width, uint32 height, void* buffer)
{
}

uint8* CGSH_Vulkan::GetRam() const
{
	return m_memoryBufferPtr;
}

Framework::CBitmap CGSH_Vulkan::GetScreenshot()
{
	return Framework::CBitmap();
}
