#pragma once

#include "../gs/GSH_Vulkan/GSH_Vulkan.h"

class CGSH_VulkanAndroid : public CGSH_Vulkan
{
public:
	CGSH_VulkanAndroid(ANativeWindow*);
	virtual ~CGSH_VulkanAndroid() = default;

	void SetWindow(ANativeWindow*);

	static FactoryFunction GetFactoryFunction(ANativeWindow*);

	void InitializeImpl() override;
	void PresentBackbuffer() override;

private:
	void CreateSurface();
	void UpdateViewport();

	ANativeWindow* m_window = nullptr;
};
