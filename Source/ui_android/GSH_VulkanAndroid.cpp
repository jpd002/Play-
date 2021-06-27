#include "GSH_VulkanAndroid.h"

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
	
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.window = m_window;
	auto result = m_instance.vkCreateAndroidSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_context->surface);
	CHECKVULKANERROR(result);

	CGSH_Vulkan::InitializeImpl();
}

void CGSH_VulkanAndroid::PresentBackbuffer()
{

}
