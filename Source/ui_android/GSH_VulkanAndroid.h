#pragma once

#include "../gs/GSH_Vulkan/GSH_Vulkan.h"
#include "NativeWindowUpdateListener.h"

class CGSH_VulkanAndroid : public CGSH_Vulkan, public INativeWindowUpdateListener
{
public:
	CGSH_VulkanAndroid(ANativeWindow*);
	virtual ~CGSH_VulkanAndroid() = default;

	void SetWindow(ANativeWindow*) override;

	static FactoryFunction GetFactoryFunction(ANativeWindow*);

	void InitializeImpl() override;
	void ReleaseImpl() override;
	void PresentBackbuffer() override;

private:
	void CreateSurface();
	void UpdateViewport();

	ANativeWindow* m_window = nullptr;
};
