#include "GsStateUtils.h"
#include "string_format.h"

static const char* g_yesNoString[2] = 
{
	"NO",
	"YES"
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

std::string CGsStateUtils::GetInputState(CGSHandler* gs)
{
	std::string result;

	CGSHandler::PRIM prim;
	prim <<= gs->GetRegisters()[GS_REG_PRIM];

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

	return result;
}

std::string CGsStateUtils::GetContextState(CGSHandler* gs, unsigned int contextId)
{
	std::string result;

	CGSHandler::TEST test;
	test <<= gs->GetRegisters()[GS_REG_TEST_1 + contextId];

	CGSHandler::ALPHA alpha;
	alpha <<= gs->GetRegisters()[GS_REG_ALPHA_1 + contextId];

	result += string_format("Depth Testing:\r\n");
	result += string_format("\tEnabled: %s\r\n", g_yesNoString[test.nDepthEnabled]);
	result += string_format("\tFunction: %s\r\n", g_depthTestFunctionString[test.nDepthMethod]);
	
	result += string_format("Alpha Testing:\r\n");
	result += string_format("\tEnabled: %s\r\n", g_yesNoString[test.nAlphaEnabled]);
	result += string_format("\tFunction: %s\r\n", g_alphaTestFunctionString[test.nAlphaMethod]);
	result += string_format("\tRef Value: 0x%0.2X\r\n", test.nAlphaRef);
	result += string_format("\tFail Op: %s\r\n", g_alphaTestFailOpString[test.nAlphaFail]);

	result += string_format("Alpha Blending:\r\n");
	result += string_format("\tFormula: (%s - %s) * %s + %s\r\n", 
		g_alphaBlendAbdCoefString[alpha.nA], g_alphaBlendAbdCoefString[alpha.nB],
		g_alphaBlendCCoefString[alpha.nC], g_alphaBlendAbdCoefString[alpha.nD]);
	result += string_format("\tFixed Value: 0x%0.2X\r\n", alpha.nFix);

	result += "\r\n";

	return result;
}
