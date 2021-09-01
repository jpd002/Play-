#include "GSH_VulkanAndroid.h"
#include <android/native_window.h>
#include "vulkan/StructDefs.h"

CGSH_VulkanAndroid::CGSH_VulkanAndroid(ANativeWindow* window)
    : m_window(window)
{
	//We'll probably need to keep 'window' as a Global JNI object
}

CGSHandler::FactoryFunction CGSH_VulkanAndroid::GetFactoryFunction(ANativeWindow* window)
{
	return [window]() { return new CGSH_VulkanAndroid(window); };
}

void CGSH_VulkanAndroid::InitializeImpl()
{
	m_instance = CreateInstance(true);

	CreateSurface();
	CGSH_Vulkan::InitializeImpl();
	UpdateViewport();
}

void CGSH_VulkanAndroid::PresentBackbuffer()
{
}

void CGSH_VulkanAndroid::SetWindow(ANativeWindow* window)
{
	m_window = window;
	SendGSCall(
			[this]() {
				CreateSurface();
				UpdateViewport();
			},
			true);
}

void CGSH_VulkanAndroid::CreateSurface()
{
	if(m_context->surface != VK_NULL_HANDLE)
	{
		m_instance.vkDestroySurfaceKHR(m_instance, m_context->surface, nullptr);
		m_context->surface = VK_NULL_HANDLE;
	}

	auto surfaceCreateInfo = Framework::Vulkan::AndroidSurfaceCreateInfoKHR();
	surfaceCreateInfo.window = m_window;
	auto result = m_instance.vkCreateAndroidSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_context->surface);
	CHECKVULKANERROR(result);
}

void CGSH_VulkanAndroid::UpdateViewport()
{
	PRESENTATION_PARAMS presentationParams;
	presentationParams.mode = PRESENTATION_MODE_FIT;
	presentationParams.windowWidth = ANativeWindow_getWidth(m_window);
	presentationParams.windowHeight = ANativeWindow_getHeight(m_window);

	SetPresentationParams(presentationParams);
}
