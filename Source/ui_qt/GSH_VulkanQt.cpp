#include "GSH_VulkanQt.h"
#include <QWindow>
#include <QVulkanInstance>

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
#ifdef USE_MACOS_MOLTENVK
	
	//Initialize for macOS
	VkInstanceCreateInfo info = {};
	const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_MVK_MACOS_SURFACE_EXTENSION_NAME };
	info.enabledExtensionCount = 2;
	info.ppEnabledExtensionNames = extensions;
	m_instance = Framework::Vulkan::CInstance(info);
	
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.pView = reinterpret_cast<const void*>(m_renderWindow->winId());
	auto result = m_instance.vkCreateMacOSSurfaceMVK(m_instance, &surfaceCreateInfo, nullptr, &m_context->surface);
	CHECKVULKANERROR(result);

#else
	
	//Generic initialization
	m_qtVulkanInstance = new QVulkanInstance();
		
#ifdef _DEBUG
	m_qtVulkanInstance->setLayers(QByteArrayList()
		<< "VK_LAYER_LUNARG_standard_validation"
		<< "VK_LAYER_GOOGLE_threading"
		<< "VK_LAYER_LUNARG_param_checker"
		<< "VK_LAYER_LUNARG_object_tracker");
#endif

	bool succeeded = m_qtVulkanInstance->create();
	Q_ASSERT(succeeded);

	m_renderWindow->setVulkanInstance(m_qtVulkanInstance);

	m_instance = Framework::Vulkan::CInstance(m_qtVulkanInstance->vkInstance());
	m_context->surface = m_qtVulkanInstance->surfaceForWindow(m_renderWindow);

#endif
	
	CGSH_Vulkan::InitializeImpl();
}

void CGSH_VulkanQt::ReleaseImpl()
{
	CGSH_Vulkan::ReleaseImpl();
#if USE_GENERIC_QTVULKAN
	delete m_qtVulkanInstance;
#endif
}

void CGSH_VulkanQt::PresentBackbuffer()
{
	if(m_renderWindow->isExposed())
	{
#if USE_GENERIC_QTVULKAN
		m_qtVulkanInstance->presentQueued(m_renderWindow);
#endif
	}
}
