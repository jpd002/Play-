#pragma once

#include "gs/GSH_Vulkan/GSH_Vulkan.h"

class QWindow;
class QVulkanInstance;

class CGSH_VulkanQt : public CGSH_Vulkan
{
public:
	CGSH_VulkanQt(QWindow*);
	virtual ~CGSH_VulkanQt() = default;

	static FactoryFunction GetFactoryFunction(QWindow*);

	void InitializeImpl() override;
	void ReleaseImpl() override;
	void PresentBackbuffer() override;

private:
	QWindow* m_renderWindow = nullptr;
	QVulkanInstance* m_qtVulkanInstance = nullptr;
};
