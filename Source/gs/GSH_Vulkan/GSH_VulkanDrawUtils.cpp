#include "GSH_VulkanDrawUtils.h"
#include "GSH_VulkanMemoryUtils.h"
#include "../GsPixelFormats.h"

using namespace GSH_Vulkan;

Nuanceur::CUintRvalue CDrawUtils::GetDepth(Nuanceur::CShaderBuilder& b, uint32 depthFormat,
                                           Nuanceur::CIntValue depthAddress, Nuanceur::CArrayUintValue memoryBuffer)
{
	switch(depthFormat)
	{
	default:
		assert(false);
	case CGSHandler::PSMZ32:
		return CMemoryUtils::Memory_Read32(b, memoryBuffer, depthAddress);
	case CGSHandler::PSMZ24:
		return CMemoryUtils::Memory_Read24(b, memoryBuffer, depthAddress);
	case CGSHandler::PSMZ16:
	case CGSHandler::PSMZ16S:
		return CMemoryUtils::Memory_Read16(b, memoryBuffer, depthAddress);
	}
}

Nuanceur::CIntRvalue CDrawUtils::ClampTexCoord(Nuanceur::CShaderBuilder& b, uint32 clampMode, Nuanceur::CIntValue texCoord, Nuanceur::CIntValue texSize,
                                               Nuanceur::CIntValue clampMin, Nuanceur::CIntValue clampMax)
{
	using namespace Nuanceur;

	switch(clampMode)
	{
	default:
		assert(false);
	case CGSHandler::CLAMP_MODE_REPEAT:
		return texCoord & (texSize - NewInt(b, 1));
	case CGSHandler::CLAMP_MODE_CLAMP:
		return Clamp(texCoord, NewInt(b, 0), texSize - NewInt(b, 1));
	case CGSHandler::CLAMP_MODE_REGION_CLAMP:
		return Clamp(texCoord, clampMin, clampMax);
	case CGSHandler::CLAMP_MODE_REGION_REPEAT:
		return (texCoord & clampMin) | clampMax;
	}
};

static Nuanceur::CFloat4Rvalue GetClutColor(Nuanceur::CShaderBuilder& b,
                                            uint32 textureFormat, uint32 clutFormat, Nuanceur::CUintValue texPixel,
                                            Nuanceur::CArrayUintValue clutBuffer, Nuanceur::CIntValue texCsa)
{
	using namespace Nuanceur;

	assert(CGsPixelFormats::IsPsmIDTEX(textureFormat));

	bool idx8 = CGsPixelFormats::IsPsmIDTEX8(textureFormat) ? 1 : 0;
	auto clutIndex = CIntLvalue(b.CreateTemporaryInt());

	if(idx8)
	{
		clutIndex = ToInt(texPixel);
	}
	else
	{
		clutIndex = ToInt(texPixel) + texCsa;
	}

	switch(clutFormat)
	{
	default:
		assert(false);
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMCT24:
	{
		auto clutIndexLo = (clutIndex & NewInt(b, 0xFF));
		auto clutIndexHi = (clutIndex & NewInt(b, 0xFF)) + NewInt(b, 0x100);
		auto clutPixelLo = Load(clutBuffer, clutIndexLo);
		auto clutPixelHi = Load(clutBuffer, clutIndexHi);
		auto clutPixel = clutPixelLo | (clutPixelHi << NewUint(b, 16));
		return CMemoryUtils::PSM32ToVec4(b, clutPixel);
	}
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
	{
		auto clutPixel = Load(clutBuffer, clutIndex);
		return CMemoryUtils::PSM16ToVec4(b, clutPixel);
	}
	}
}

Nuanceur::CFloat4Rvalue CDrawUtils::GetTextureColor(Nuanceur::CShaderBuilder& b, uint32 textureFormat, uint32 clutFormat,
                                                    Nuanceur::CInt2Value texelPos, Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CArrayUintValue clutBuffer,
                                                    Nuanceur::CImageUint2DValue texSwizzleTable, Nuanceur::CIntValue texBufAddress, Nuanceur::CIntValue texBufWidth,
                                                    Nuanceur::CIntValue texCsa)
{
	using namespace Nuanceur;

	switch(textureFormat)
	{
	default:
		assert(false);
		[[fallthrough]];
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMZ32:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read32(b, memoryBuffer, texAddress);
		return CMemoryUtils::PSM32ToVec4(b, texPixel);
	}
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMCT24_UNK:
	case CGSHandler::PSMZ24:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read24(b, memoryBuffer, texAddress);
		return CMemoryUtils::PSM32ToVec4(b, texPixel);
	}
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
	case CGSHandler::PSMZ16:
	case CGSHandler::PSMZ16S:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read16(b, memoryBuffer, texAddress);
		return CMemoryUtils::PSM16ToVec4(b, texPixel);
	}
	case CGSHandler::PSMT8:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMT8>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read8(b, memoryBuffer, texAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT4:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress_PSMT4(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read4(b, memoryBuffer, texAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT8H:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read8(b, memoryBuffer, texAddress + NewInt(b, 3));
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT4HL:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texNibAddress = (texAddress + NewInt(b, 3)) * NewInt(b, 2);
		auto texPixel = CMemoryUtils::Memory_Read4(b, memoryBuffer, texNibAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT4HH:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texNibAddress = ((texAddress + NewInt(b, 3)) * NewInt(b, 2)) | NewInt(b, 1);
		auto texPixel = CMemoryUtils::Memory_Read4(b, memoryBuffer, texNibAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	}
}

void CDrawUtils::ExpandAlpha(Nuanceur::CShaderBuilder& b, uint32 textureFormat, uint32 clutFormat,
                             uint32 texBlackIsTransparent, Nuanceur::CFloat4Lvalue& textureColor,
                             Nuanceur::CFloatValue textureA0, Nuanceur::CFloatValue textureA1)
{
	using namespace Nuanceur;

	bool requiresExpansion = false;
	if(CGsPixelFormats::IsPsmIDTEX(textureFormat))
	{
		requiresExpansion =
		    (clutFormat == CGSHandler::PSMCT16) ||
		    (clutFormat == CGSHandler::PSMCT16S);
	}
	else
	{
		requiresExpansion =
		    (textureFormat == CGSHandler::PSMCT24) ||
		    (textureFormat == CGSHandler::PSMCT16) ||
		    (textureFormat == CGSHandler::PSMCT16S);
	}

	if(!requiresExpansion)
	{
		return;
	}

	auto alpha = Mix(textureA0, textureA1, textureColor->w());
	textureColor = NewFloat4(textureColor->xyz(), alpha);

	if(texBlackIsTransparent)
	{
		//Add rgb and check if it is zero (assume rgb is positive)
		//Set alpha to 0 if it is
		auto colorSum = textureColor->x() + textureColor->y() + textureColor->z();
		BeginIf(b, colorSum == NewFloat(b, 0));
		{
			textureColor = NewFloat4(textureColor->xyz(), NewFloat(b, 0));
		}
		EndIf(b);
	}
}

Nuanceur::CInt3Rvalue CDrawUtils::GetAlphaABD(Nuanceur::CShaderBuilder& b, uint32 alphaABD,
                                              Nuanceur::CInt4Value srcColor, Nuanceur::CInt4Value dstColor)
{
	switch(alphaABD)
	{
	default:
		assert(false);
	case CGSHandler::ALPHABLEND_ABD_CS:
		return srcColor->xyz();
	case CGSHandler::ALPHABLEND_ABD_CD:
		return dstColor->xyz();
	case CGSHandler::ALPHABLEND_ABD_ZERO:
		return NewInt3(b, 0, 0, 0);
	}
}

Nuanceur::CInt3Rvalue CDrawUtils::GetAlphaC(Nuanceur::CShaderBuilder& b, uint32 alphaC,
                                            Nuanceur::CInt4Value srcColor, Nuanceur::CInt4Value dstColor, Nuanceur::CIntValue alphaFix)
{
	switch(alphaC)
	{
	default:
		assert(false);
	case CGSHandler::ALPHABLEND_C_AS:
		return srcColor->www();
	case CGSHandler::ALPHABLEND_C_AD:
		return dstColor->www();
	case CGSHandler::ALPHABLEND_C_FIX:
		return alphaFix->xxx();
	}
}

void CDrawUtils::AlphaTest(Nuanceur::CShaderBuilder& b,
                           uint32 alphaTestFunction, uint32 alphaTestFailAction,
                           Nuanceur::CInt4Value srcIColor, Nuanceur::CIntValue alphaRef,
                           Nuanceur::CBoolLvalue writeColor, Nuanceur::CBoolLvalue writeDepth, Nuanceur::CBoolLvalue writeAlpha)
{
	using namespace Nuanceur;

	auto srcAlpha = srcIColor->w();
	auto alphaTestResult = CBoolLvalue(b.CreateVariableBool("alphaTestResult"));
	switch(alphaTestFunction)
	{
	default:
		assert(false);
	case CGSHandler::ALPHA_TEST_ALWAYS:
		alphaTestResult = NewBool(b, true);
		break;
	case CGSHandler::ALPHA_TEST_NEVER:
		alphaTestResult = NewBool(b, false);
		break;
	case CGSHandler::ALPHA_TEST_LESS:
		alphaTestResult = srcAlpha < alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_LEQUAL:
		alphaTestResult = srcAlpha <= alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_EQUAL:
		alphaTestResult = srcAlpha == alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_GEQUAL:
		alphaTestResult = srcAlpha >= alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_GREATER:
		alphaTestResult = srcAlpha > alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_NOTEQUAL:
		alphaTestResult = srcAlpha != alphaRef;
		break;
	}

	BeginIf(b, !alphaTestResult);
	{
		switch(alphaTestFailAction)
		{
		default:
			assert(false);
		case CGSHandler::ALPHA_TEST_FAIL_KEEP:
			writeColor = NewBool(b, false);
			writeDepth = NewBool(b, false);
			break;
		case CGSHandler::ALPHA_TEST_FAIL_FBONLY:
			writeDepth = NewBool(b, false);
			break;
		case CGSHandler::ALPHA_TEST_FAIL_ZBONLY:
			writeColor = NewBool(b, false);
			break;
		case CGSHandler::ALPHA_TEST_FAIL_RGBONLY:
			writeDepth = NewBool(b, false);
			writeAlpha = NewBool(b, false);
			break;
		}
	}
	EndIf(b);
}

void CDrawUtils::DestinationAlphaTest(Nuanceur::CShaderBuilder& b, uint32 framebufferFormat,
                                      uint32 dstAlphaTestRef, Nuanceur::CUintValue dstPixel,
                                      Nuanceur::CBoolLvalue writeColor, Nuanceur::CBoolLvalue writeDepth)
{
	using namespace Nuanceur;

	auto alphaBit = CUintLvalue(b.CreateTemporaryUint());
	switch(framebufferFormat)
	{
	case CGSHandler::PSMCT32:
		alphaBit = dstPixel & NewUint(b, 0x80000000);
		break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
		alphaBit = dstPixel & NewUint(b, 0x8000);
		break;
	default:
		assert(false);
		break;
	}

	auto dstAlphaTestResult = CBoolLvalue(b.CreateVariableBool("dstAlphaTestResult"));
	if(dstAlphaTestRef)
	{
		//Pixels with alpha bit set pass
		dstAlphaTestResult = (alphaBit != NewUint(b, 0));
	}
	else
	{
		dstAlphaTestResult = (alphaBit == NewUint(b, 0));
	}

	BeginIf(b, !dstAlphaTestResult);
	{
		writeColor = NewBool(b, false);
		writeDepth = NewBool(b, false);
	}
	EndIf(b);
}

void CDrawUtils::WriteToFramebuffer(Nuanceur::CShaderBuilder& b, uint32 framebufferFormat,
                                    Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CIntValue fbAddress, Nuanceur::CUintValue srcPixel)
{
	switch(framebufferFormat)
	{
	default:
		assert(false);
		[[fallthrough]];
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMZ32:
	{
		CMemoryUtils::Memory_Write32(b, memoryBuffer, fbAddress, srcPixel);
	}
	break;
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMZ24:
	{
		auto dstPixel = srcPixel & NewUint(b, 0xFFFFFF);
		CMemoryUtils::Memory_Write24(b, memoryBuffer, fbAddress, dstPixel);
	}
	break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
	{
		auto dstPixel = srcPixel & NewUint(b, 0xFFFF);
		CMemoryUtils::Memory_Write16(b, memoryBuffer, fbAddress, dstPixel);
	}
	break;
	}
}

void CDrawUtils::WriteToDepthbuffer(Nuanceur::CShaderBuilder& b, uint32 depthbufferFormat,
                                    Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CIntValue depthAddress, Nuanceur::CUintValue srcDepth)
{
	switch(depthbufferFormat)
	{
	case CGSHandler::PSMZ32:
	{
		CMemoryUtils::Memory_Write32(b, memoryBuffer, depthAddress, srcDepth);
	}
	break;
	case CGSHandler::PSMZ24:
	{
		auto dstDepth = srcDepth & NewUint(b, 0xFFFFFF);
		CMemoryUtils::Memory_Write24(b, memoryBuffer, depthAddress, dstDepth);
	}
	break;
	case CGSHandler::PSMZ16:
	case CGSHandler::PSMZ16S:
	{
		auto dstDepth = srcDepth & NewUint(b, 0xFFFF);
		CMemoryUtils::Memory_Write16(b, memoryBuffer, depthAddress, dstDepth);
	}
	break;
	default:
		assert(false);
		break;
	}
}
