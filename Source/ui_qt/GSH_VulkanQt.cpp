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
	m_surface = m_qtVulkanInstance->surfaceForWindow(m_renderWindow);

	CGSH_Vulkan::InitializeImpl();
}

void CGSH_VulkanQt::ReleaseImpl()
{
	CGSH_Vulkan::ReleaseImpl();
	delete m_qtVulkanInstance;
}

void CGSH_VulkanQt::PresentBackbuffer()
{
	if(m_renderWindow->isExposed())
	{
		m_qtVulkanInstance->presentQueued(m_renderWindow);
	}
}
