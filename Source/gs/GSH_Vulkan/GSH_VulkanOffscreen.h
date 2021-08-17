#pragma once

#include "GSH_Vulkan.h"

class CGSH_VulkanOffscreen : public CGSH_Vulkan
{
public:
	void PresentBackbuffer() override;

protected:
	void InitializeImpl() override;
};
