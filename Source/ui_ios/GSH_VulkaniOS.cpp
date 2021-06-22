#include <cassert>
#include "GSH_VulkaniOS.h"
#include "vulkan/Loader.h"

CGSH_VulkaniOS::CGSH_VulkaniOS(CAMetalLayer* layer)
	: m_layer(layer)
{
}

CGSHandler::FactoryFunction CGSH_VulkaniOS::GetFactoryFunction(CAMetalLayer* layer)
{
	return [layer]() { return new CGSH_VulkaniOS(layer); };
}

void CGSH_VulkaniOS::InitializeImpl()
{
	VkInstanceCreateInfo info = {};
	const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_MVK_IOS_SURFACE_EXTENSION_NAME };
	info.enabledExtensionCount = 2;
	info.ppEnabledExtensionNames = extensions;
	m_instance = Framework::Vulkan::CInstance(info);
	
	VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.pView = (__bridge void*)m_layer;
	auto result = m_instance.vkCreateIOSSurfaceMVK(m_instance, &surfaceCreateInfo, nullptr, &m_context->surface);
	CHECKVULKANERROR(result);

	CGSH_Vulkan::InitializeImpl();
	
	{
		CGSize drawableSize = m_layer.drawableSize;

		PRESENTATION_PARAMS presentationParams;
		presentationParams.mode = PRESENTATION_MODE_FIT;
		presentationParams.windowWidth = drawableSize.width;
		presentationParams.windowHeight = drawableSize.height;

		SetPresentationParams(presentationParams);
	}
}

void CGSH_VulkaniOS::PresentBackbuffer()
{
	
}
