#include "GSH_VulkanQt.h"
#include "vulkan/StructDefs.h"
#include <QWindow>

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

	std::vector<const char*> layers;
#ifdef _DEBUG
	layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif

	auto appInfo = Framework::Vulkan::ApplicationInfo();
	appInfo.pApplicationName = "Play!";
	appInfo.pEngineName      = "Play!";
	//Added fragment shader interlock in 1.1.110
	appInfo.apiVersion       = VK_MAKE_VERSION(1, 1, 110);

	instanceCreateInfo.pApplicationInfo        = &appInfo;
	instanceCreateInfo.enabledExtensionCount   = extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	instanceCreateInfo.enabledLayerCount       = layers.size();
	instanceCreateInfo.ppEnabledLayerNames     = layers.data();
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
