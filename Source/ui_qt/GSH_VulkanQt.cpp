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
	m_instance = CreateInstance(true);

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
