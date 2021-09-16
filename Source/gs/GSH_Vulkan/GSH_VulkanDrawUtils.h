#pragma once

#include "nuanceur/Builder.h"

namespace GSH_Vulkan
{
	class CDrawUtils
	{
	public:
		static Nuanceur::CUintRvalue GetDepth(Nuanceur::CShaderBuilder& b, uint32 depthFormat,
		                                      Nuanceur::CIntValue depthAddress, Nuanceur::CArrayUintValue memoryBuffer);
		static Nuanceur::CIntRvalue ClampTexCoord(Nuanceur::CShaderBuilder& b, uint32 clampMode, Nuanceur::CIntValue texCoord,
		                                          Nuanceur::CIntValue texSize, Nuanceur::CIntValue clampMin, Nuanceur::CIntValue clampMax);
		static Nuanceur::CFloat4Rvalue GetTextureColor(Nuanceur::CShaderBuilder& b, uint32 textureFormat, uint32 clutFormat,
		                                               Nuanceur::CInt2Value texelPos, Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CArrayUintValue clutBuffer,
		                                               Nuanceur::CImageUint2DValue texSwizzleTable, Nuanceur::CIntValue texBufAddress, Nuanceur::CIntValue texBufWidth,
		                                               Nuanceur::CIntValue texCsa);
		static void ExpandAlpha(Nuanceur::CShaderBuilder& b, uint32 textureFormat, uint32 clutFormat,
		                        uint32 texBlackIsTransparent, Nuanceur::CFloat4Lvalue& textureColor,
		                        Nuanceur::CFloatValue textureA0, Nuanceur::CFloatValue textureA1);

		static Nuanceur::CInt3Rvalue GetAlphaABD(Nuanceur::CShaderBuilder& b, uint32 alphaABD,
		                                         Nuanceur::CInt4Value srcColor, Nuanceur::CInt4Value dstColor);
		static Nuanceur::CInt3Rvalue GetAlphaC(Nuanceur::CShaderBuilder& b, uint32 alphaC,
		                                       Nuanceur::CInt4Value srcColor, Nuanceur::CInt4Value dstColor, Nuanceur::CIntValue alphaFix);
		static void AlphaTest(Nuanceur::CShaderBuilder& b,
		                      uint32 alphaTestFunction, uint32 alphaTestFailAction,
		                      Nuanceur::CInt4Value srcIColor, Nuanceur::CIntValue alphaRef,
		                      Nuanceur::CBoolLvalue writeColor, Nuanceur::CBoolLvalue writeDepth, Nuanceur::CBoolLvalue writeAlpha);

		static void DestinationAlphaTest(Nuanceur::CShaderBuilder& b, uint32 framebufferFormat,
		                                 uint32 dstAlphaTestRef, Nuanceur::CUintValue dstPixel,
		                                 Nuanceur::CBoolLvalue writeColor, Nuanceur::CBoolLvalue writeDepth);

		static void WriteToFramebuffer(Nuanceur::CShaderBuilder& b, uint32 framebufferFormat,
		                               Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CIntValue fbAddress, Nuanceur::CUintValue srcPixel);
		static void WriteToDepthbuffer(Nuanceur::CShaderBuilder& b, uint32 depthbufferFormat,
		                               Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CIntValue depthAddress, Nuanceur::CUintValue srcDepth);
	};
}
