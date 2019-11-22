#include "GSH_VulkanMemoryUtils.h"

using namespace GSH_Vulkan;

Nuanceur::CFloat4Rvalue CMemoryUtils::PSM32ToVec4(Nuanceur::CShaderBuilder& b, Nuanceur::CUintValue inputColor)
{
	auto colorR = (inputColor >> NewUint(b,  0)) & NewUint(b, 0xFF);
	auto colorG = (inputColor >> NewUint(b,  8)) & NewUint(b, 0xFF);
	auto colorB = (inputColor >> NewUint(b, 16)) & NewUint(b, 0xFF);
	auto colorA = (inputColor >> NewUint(b, 24)) & NewUint(b, 0xFF);

	auto colorFloatR = ToFloat(colorR) / NewFloat(b, 255.f);
	auto colorFloatG = ToFloat(colorG) / NewFloat(b, 255.f);
	auto colorFloatB = ToFloat(colorB) / NewFloat(b, 255.f);
	auto colorFloatA = ToFloat(colorA) / NewFloat(b, 255.f);

	return NewFloat4(colorFloatR, colorFloatG, colorFloatB, colorFloatA);
}

Nuanceur::CUintRvalue CMemoryUtils::Vec4ToPSM32(Nuanceur::CShaderBuilder& b, Nuanceur::CFloat4Value inputColor)
{
	auto colorR = ToUint(inputColor->x() * NewFloat(b, 255.f)) << NewUint(b,  0);
	auto colorG = ToUint(inputColor->y() * NewFloat(b, 255.f)) << NewUint(b,  8);
	auto colorB = ToUint(inputColor->z() * NewFloat(b, 255.f)) << NewUint(b, 16);
	auto colorA = ToUint(inputColor->w() * NewFloat(b, 255.f)) << NewUint(b, 24);
	return colorR | colorG | colorB | colorA;
}
