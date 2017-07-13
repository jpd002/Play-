#include "GsStateUtils.h"
#include "string_format.h"
#include "../GSH_Direct3D9.h"

static const char* g_yesNoString[2] = 
{
	"NO",
	"YES"
};

static const char* g_pixelFormats[0x40] =
{
	//0x00
	"PSMCT32",		"PSMCT24",		"PSMCT16",		"(INVALID)",
	//0x04
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"(INVALID)",
	//0x08
	"(INVALID)",	"(INVALID)",	"PSMCT16S",		"(INVALID)",
	//0x0C
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"(INVALID)",

	//0x10
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"PSMT8",
	//0x14
	"PSMT4",		"(INVALID)",	"(INVALID)",	"(INVALID)",
	//0x18
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"PSMT8H",
	//0x1C
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"(INVALID)",

	//0x20
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"(INVALID)",
	//0x24
	"PSMT4HL",		"(INVALID)",	"(INVALID)",	"(INVALID)",
	//0x28
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"(INVALID)",
	//0x2C
	"PSMT4HH",		"(INVALID)",	"(INVALID)",	"(INVALID)",

	//0x30
	"PSMZ32",		"PSMZ24",		"PSMZ16",		"(INVALID)",
	//0x34
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"(INVALID)",
	//0x38
	"(INVALID)",	"(INVALID)",	"PSMZ16S",		"(INVALID)",
	//0x3C
	"(INVALID)",	"(INVALID)",	"(INVALID)",	"(INVALID)",
};

static const char* g_primitiveTypeString[8] =
{
	"POINT",
	"LINE",
	"LINESTRIP",
	"TRIANGLE",
	"TRIANGLESTRIP",
	"TRIANGLEFAN",
	"SPRITE",
	"(INVALID)"
};

static const char* g_depthTestFunctionString[4] =
{
	"NEVER",
	"ALWAYS",
	"GEQUAL",
	"GREATER"
};

static const char* g_alphaTestFunctionString[8] =
{
	"NEVER",
	"ALWAYS",
	"LESS",
	"LEQUAL",
	"EQUAL",
	"GEQUAL",
	"GREATER",
	"NOTEQUAL"
};

static const char* g_alphaTestFailOpString[4] =
{
	"KEEP",
	"FB_ONLY",
	"ZB_ONLY",
	"RGB_ONLY"
};

static const char* g_alphaBlendAbdCoefString[4] =
{
	"Cs",
	"Cd",
	"0",
	"(INVALID)"
};

static const char* g_alphaBlendCCoefString[4] =
{
	"As",
	"Ad",
	"FIX",
	"(INVALID)"
};

static const char* g_textureFunctionString[4] =
{
	"MODULATE",
	"DECAL",
	"HIGHLIGHT",
	"HIGHLIGHT2"
};

static const char* g_textureClutLoadControlString[8] =
{
	"DO NOT LOAD",
	"LOAD",
	"LOAD AND COPY CBP TO CBP0",
	"LOAD AND COPY CBP TO CBP1",
	"LOAD IF CBP != CBP0 AND COPY CBP TO CBP0",
	"LOAD IF CBP != CBP1 AND COPY CBP TO CBP1",
	"(INVALID)",
	"(INVALID)",
};

static const char* g_textureMagFilterString[2] =
{
	"NEAREST",
	"LINEAR"
};

static const char* g_textureMinFilterString[8] =
{
	"NEAREST",
	"LINEAR",
	"NEAREST_MIPMAP_NEAREST",
	"NEAREST_MIPMAP_LINEAR",
	"LINEAR_MIPMAP_NEAREST",
	"LINEAR_MIPMAP_LINEAR",
	"(INVALID)",
	"(INVALID)"
};

static const char* g_wrapModeString[4] =
{
	"REPEAT",
	"CLAMP",
	"REGION_CLAMP",
	"REGION_REPEAT"
};

std::string CGsStateUtils::GetInputState(CGSHandler* gs)
{
	std::string result;

	auto prim = make_convertible<CGSHandler::PRIM>(gs->GetRegisters()[GS_REG_PRIM]);
	auto xyOffset = make_convertible<CGSHandler::XYOFFSET>(gs->GetRegisters()[GS_REG_XYOFFSET_1 + prim.nContext]);

	bool usePrmode = (gs->GetRegisters()[GS_REG_PRMODECONT] & 1) == 0;

	result += string_format("Use PRMODE: %s\r\n", g_yesNoString[usePrmode]);
	
	result += string_format("Primitive:\r\n");
	result += string_format("\tContext: %d\r\n", prim.nContext + 1);
	result += string_format("\tType: %s\r\n", g_primitiveTypeString[prim.nType]);
	result += string_format("\tUse Gouraud: %s\r\n", g_yesNoString[prim.nShading]);
	result += string_format("\tUse Texture: %s\r\n", g_yesNoString[prim.nTexture]);
	result += string_format("\tUse Fog: %s\r\n", g_yesNoString[prim.nFog]);
	result += string_format("\tUse Alpha Blending: %s\r\n", g_yesNoString[prim.nAlpha]);
	result += string_format("\tUse Antialiasing: %s\r\n", g_yesNoString[prim.nAntiAliasing]);
	result += string_format("\tUse UV: %s\r\n", g_yesNoString[prim.nUseUV]);
	result += string_format("\tUse Fixed Interpolation: %s\r\n", g_yesNoString[prim.nUseFloat]);

	result += "\r\n";

	auto vertices = static_cast<CGSH_Direct3D9*>(gs)->GetInputVertices();

	result += string_format("Positions:\r\n");
	result += string_format("\t                 PosX        PosY        PosZ        OfsX        OfsY\r\n");
	for(unsigned int i = 0; i < 3; i++)
	{
		auto vertex = vertices[i];
		float posX = static_cast<float>((vertex.nPosition >>  0) & 0xFFFF) / 16;
		float posY = static_cast<float>((vertex.nPosition >> 16) & 0xFFFF) / 16;
		uint32 posZ = static_cast<uint32>(vertex.nPosition >> 32);
		result += string_format("\tVertex %i:  %+10.4f  %+10.4f  0x%08X  %+10.4f  %+10.4f\r\n", 
			i, posX, posY, posZ, posX - xyOffset.GetX(), posY - xyOffset.GetY());
	}

	result += "\r\n";

	result += string_format("Texture Coordinates:\r\n");
	result += string_format("\t                    S           T           Q           U           V\r\n");
	for(unsigned int i = 0; i < 3; i++)
	{
		auto vertex = vertices[i];
		auto st = make_convertible<CGSHandler::ST>(vertex.nST);
		auto rgbaq = make_convertible<CGSHandler::RGBAQ>(vertex.nRGBAQ);
		auto uv = make_convertible<CGSHandler::UV>(vertex.nUV);
		result += string_format("\tVertex %i:  %+10.4f  %+10.4f  %+10.4f  %+10.4f  %+10.4f\r\n", 
			i, st.nS, st.nT, rgbaq.nQ, uv.GetU(), uv.GetV());
	}

	result += string_format("Color:\r\n");
	result += string_format("\t                    R           G           B           A\r\n");
	for(unsigned int i = 0; i < 3; i++)
	{
		auto vertex = vertices[i];
		auto rgbaq = make_convertible<CGSHandler::RGBAQ>(vertex.nRGBAQ);
		result += string_format("\tVertex %i:        0x%02X        0x%02X        0x%02X        0x%02X\r\n", 
			i, rgbaq.nR, rgbaq.nG, rgbaq.nB, rgbaq.nA);
	}

	result += string_format("Fog:\r\n");
	result += string_format("\t                    F\r\n");
	for(unsigned int i = 0; i < 3; i++)
	{
		auto vertex = vertices[i];
		result += string_format("\tVertex %i:        0x%02X\r\n", 
			i, vertex.nFog);
	}

	return result;
}

std::string CGsStateUtils::GetContextState(CGSHandler* gs, unsigned int contextId)
{
	std::string result;

	{
		auto frame = make_convertible<CGSHandler::FRAME>(gs->GetRegisters()[GS_REG_FRAME_1 + contextId]);
		result += string_format("Frame Buffer:\r\n");
		result += string_format("\tPtr: 0x%08X\r\n", frame.GetBasePtr());
		result += string_format("\tWidth: %d\r\n", frame.GetWidth());
		result += string_format("\tFormat: %s\r\n", g_pixelFormats[frame.nPsm]);
		result += string_format("\tWrite Mask: 0x%08X\r\n", ~frame.nMask);
	}

	{
		auto zbuf = make_convertible<CGSHandler::ZBUF>(gs->GetRegisters()[GS_REG_ZBUF_1 + contextId]);
		result += string_format("Depth Buffer:\r\n");
		result += string_format("\tPtr: 0x%08X\r\n", zbuf.GetBasePtr());
		result += string_format("\tFormat: %s\r\n", g_pixelFormats[zbuf.nPsm | 0x30]);
		result += string_format("\tWrite Enabled: %s\r\n", g_yesNoString[(zbuf.nMask == 0)]);
	}

	{
		auto tex0 = make_convertible<CGSHandler::TEX0>(gs->GetRegisters()[GS_REG_TEX0_1 + contextId]);
		auto tex1 = make_convertible<CGSHandler::TEX1>(gs->GetRegisters()[GS_REG_TEX1_1 + contextId]);
		result += string_format("Texture:\r\n");
		result += string_format("\tPtr: 0x%08X\r\n", tex0.GetBufPtr());
		result += string_format("\tBuffer Width: %d\r\n", tex0.GetBufWidth());
		result += string_format("\tFormat: %s\r\n", g_pixelFormats[tex0.nPsm]);
		result += string_format("\tWidth: %d\r\n", tex0.GetWidth());
		result += string_format("\tHeight: %d\r\n", tex0.GetHeight());
		result += string_format("\tHas Alpha Component: %s\r\n", g_yesNoString[tex0.nColorComp]);
		result += string_format("\tFunction: %s\r\n", g_textureFunctionString[tex0.nFunction]);
		result += string_format("\tCLUT Ptr: 0x%08X\r\n", tex0.GetCLUTPtr());
		result += string_format("\tCLUT Format: %s\r\n", g_pixelFormats[tex0.nCPSM]);
		result += string_format("\tCLUT Storage Mode: %d\r\n", tex0.nCSM + 1);
		result += string_format("\tCLUT Entry Offset: %d\r\n", tex0.nCSA * 16);
		result += string_format("\tCLUT Load Control: %s\r\n", g_textureClutLoadControlString[tex0.nCLD]);
		result += string_format("\tLOD Use Fixed Value: %s\r\n", g_yesNoString[tex1.nLODMethod]);
		result += string_format("\tMaximum Mip Level: %d\r\n", tex1.nMaxMip);
		result += string_format("\tMag Filter: %s\r\n", g_textureMagFilterString[tex1.nMagFilter]);
		result += string_format("\tMin Filter: %s\r\n", g_textureMinFilterString[tex1.nMinFilter]);
		result += string_format("\tUse Automatic Mip Address: %s\r\n", g_yesNoString[tex1.nMipBaseAddr]);
		result += string_format("\tLOD L: %d\r\n", tex1.nLODL);
		result += string_format("\tLOD K: %f\r\n", tex1.GetK());
	}

	{
		auto miptbp1 = make_convertible<CGSHandler::MIPTBP1>(gs->GetRegisters()[GS_REG_MIPTBP1_1 + contextId]);
		auto miptbp2 = make_convertible<CGSHandler::MIPTBP2>(gs->GetRegisters()[GS_REG_MIPTBP2_1 + contextId]);
		result += string_format("Mipmap:\r\n");
		result += string_format("\tLevel 1: 0x%08X, %d\r\n", miptbp1.GetTbp1(), miptbp1.GetTbw1());
		result += string_format("\tLevel 2: 0x%08X, %d\r\n", miptbp1.GetTbp2(), miptbp1.GetTbw2());
		result += string_format("\tLevel 3: 0x%08X, %d\r\n", miptbp1.GetTbp3(), miptbp1.GetTbw3());
		result += string_format("\tLevel 4: 0x%08X, %d\r\n", miptbp2.GetTbp4(), miptbp2.GetTbw4());
		result += string_format("\tLevel 5: 0x%08X, %d\r\n", miptbp2.GetTbp5(), miptbp2.GetTbw5());
		result += string_format("\tLevel 6: 0x%08X, %d\r\n", miptbp2.GetTbp6(), miptbp2.GetTbw6());
	}

	{
		auto texA = make_convertible<CGSHandler::TEXA>(gs->GetRegisters()[GS_REG_TEXA]);
		result += string_format("Texture Alpha:\r\n");
		result += string_format("\tAlpha 0: 0x%02X\r\n", texA.nTA0);
		result += string_format("\tAlpha 1: 0x%02X\r\n", texA.nTA1);
		result += string_format("\tBlack Is Transparent: %s\r\n", g_yesNoString[texA.nAEM]);
	}

	{
		auto clamp = make_convertible<CGSHandler::CLAMP>(gs->GetRegisters()[GS_REG_CLAMP_1 + contextId]);
		result += string_format("Clamp:\r\n");
		result += string_format("\tWrap Mode S: %s\r\n", g_wrapModeString[clamp.nWMS]);
		result += string_format("\tWrap Mode T: %s\r\n", g_wrapModeString[clamp.nWMT]);
		result += string_format("\tMin U / U Mask: %4d / 0x%03X\r\n", clamp.GetMinU(), clamp.GetMinU());
		result += string_format("\tMax U / U Fix : %4d / 0x%03X\r\n", clamp.GetMaxU(), clamp.GetMaxU());
		result += string_format("\tMin V / V Mask: %4d / 0x%03X\r\n", clamp.GetMinV(), clamp.GetMinV());
		result += string_format("\tMax V / V Fix : %4d / 0x%03X\r\n", clamp.GetMaxV(), clamp.GetMaxV());
	}

	{
		auto test = make_convertible<CGSHandler::TEST>(gs->GetRegisters()[GS_REG_TEST_1 + contextId]);

		result += string_format("Depth Testing:\r\n");
		result += string_format("\tEnabled: %s\r\n", g_yesNoString[test.nDepthEnabled]);
		result += string_format("\tFunction: %s\r\n", g_depthTestFunctionString[test.nDepthMethod]);
	
		result += string_format("Alpha Testing:\r\n");
		result += string_format("\tEnabled: %s\r\n", g_yesNoString[test.nAlphaEnabled]);
		result += string_format("\tFunction: %s\r\n", g_alphaTestFunctionString[test.nAlphaMethod]);
		result += string_format("\tRef Value: 0x%02X\r\n", test.nAlphaRef);
		result += string_format("\tFail Op: %s\r\n", g_alphaTestFailOpString[test.nAlphaFail]);
	}

	{
		auto alpha = make_convertible<CGSHandler::ALPHA>(gs->GetRegisters()[GS_REG_ALPHA_1 + contextId]);
		result += string_format("Alpha Blending:\r\n");
		result += string_format("\tA: %d, B: %d, C: %d, D: %d\r\n", alpha.nA, alpha.nB, alpha.nC, alpha.nD);
		result += string_format("\tFormula: (%s - %s) * %s + %s\r\n", 
			g_alphaBlendAbdCoefString[alpha.nA], g_alphaBlendAbdCoefString[alpha.nB],
			g_alphaBlendCCoefString[alpha.nC], g_alphaBlendAbdCoefString[alpha.nD]);
		result += string_format("\tFixed Value: 0x%02X\r\n", alpha.nFix);
	}

	result += "\r\n";

	return result;
}
