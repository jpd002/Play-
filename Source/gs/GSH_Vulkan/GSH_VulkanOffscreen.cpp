#include "GSH_VulkanOffscreen.h"

void CGSH_VulkanOffscreen::InitializeImpl()
{
	m_instance = CreateInstance(true);
	CGSH_Vulkan::InitializeImpl();
}

void CGSH_VulkanOffscreen::PresentBackbuffer()
{
}
