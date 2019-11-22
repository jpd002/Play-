#pragma once

#include "nuanceur/Builder.h"

namespace GSH_Vulkan
{
	class CMemoryUtils
	{
	public:
		static Nuanceur::CFloat4Rvalue PSM32ToVec4(Nuanceur::CShaderBuilder&, Nuanceur::CUintValue);

		static Nuanceur::CUintRvalue Vec4ToPSM32(Nuanceur::CShaderBuilder&, Nuanceur::CFloat4Value);
	};
}
