#include "GSH_VulkanQt.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Loader.h"
#include <QWindow>

#ifdef __APPLE__
#include <MoltenVK/vk_mvk_moltenvk.h>
#endif

#ifdef __linux__
#include <QX11Info>
#endif

CGSH_VulkanQt::CGSH_VulkanQt(QWindow* renderWindow)
    : m_renderWindow(renderWindow)
{
}

CGSH_VulkanQt::FactoryFunction CGSH_VulkanQt::GetFactoryFunction(QWindow* renderWindow)
{
	return [renderWindow]() { return new CGSH_VulkanQt(renderWindow); };
}

void CGSH_VulkanQt::InitializeImpl()
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
	layers.push_back("VK_LAYER_LUNARG_standard_validation");
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
	m_instance = Framework::Vulkan::CInstance(instanceCreateInfo);

#ifdef _WIN32
	auto surfaceCreateInfo = Framework::Vulkan::Win32SurfaceCreateInfoKHR();
	surfaceCreateInfo.hwnd = reinterpret_cast<HWND>(m_renderWindow->winId());
	auto result = m_instance.vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_context->surface);
	CHECKVULKANERROR(result);
#endif

#ifdef __APPLE__
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.pView = reinterpret_cast<const void*>(m_renderWindow->winId());
	auto result = m_instance.vkCreateMacOSSurfaceMVK(m_instance, &surfaceCreateInfo, nullptr, &m_context->surface);
	CHECKVULKANERROR(result);

	DisableSyncQueueSubmits();
#endif

#ifdef __linux__
	auto surfaceCreateInfo = Framework::Vulkan::XcbSurfaceCreateInfoKHR();
	surfaceCreateInfo.window = static_cast<xcb_window_t>(m_renderWindow->winId());
	surfaceCreateInfo.connection = QX11Info::connection();
	auto result = m_instance.vkCreateXcbSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_context->surface);
	CHECKVULKANERROR(result);
#endif

	CGSH_Vulkan::InitializeImpl();
}

void CGSH_VulkanQt::ReleaseImpl()
{
	CGSH_Vulkan::ReleaseImpl();
	m_instance.vkDestroySurfaceKHR(m_instance, m_context->surface, nullptr);
}

void CGSH_VulkanQt::PresentBackbuffer()
{
}

void CGSH_VulkanQt::DisableSyncQueueSubmits()
{
#ifdef __APPLE__
	auto result = VK_SUCCESS;

	auto vkGetMoltenVKConfigurationMVK = reinterpret_cast<PFN_vkGetMoltenVKConfigurationMVK>(Framework::Vulkan::CLoader::GetInstance().GetLibraryProcAddr("vkGetMoltenVKConfigurationMVK"));
	auto vkSetMoltenVKConfigurationMVK = reinterpret_cast<PFN_vkSetMoltenVKConfigurationMVK>(Framework::Vulkan::CLoader::GetInstance().GetLibraryProcAddr("vkSetMoltenVKConfigurationMVK"));

	MVKConfiguration config;
	size_t configSize = sizeof(MVKConfiguration);
	result = vkGetMoltenVKConfigurationMVK(m_instance, &config, &configSize);
	CHECKVULKANERROR(result);

	config.synchronousQueueSubmits = VK_FALSE;

	result = vkSetMoltenVKConfigurationMVK(m_instance, &config, &configSize);
	CHECKVULKANERROR(result);
#endif
}
