#include "GSH_OpenGL.h"
#include <assert.h>
#include <sstream>

#ifdef GLES_COMPATIBILITY
#define GLSL_VERSION "#version 300 es"
#else
#define GLSL_VERSION "#version 150"
#endif

static const char* s_andFunction =
    "float and(int a, int b)\r\n"
    "{\r\n"
    "	int r = 0;\r\n"
    "	int ha, hb;\r\n"
    "	\r\n"
    "	int m = int(min(float(a), float(b)));\r\n"
    "	\r\n"
    "	for(int k = 1; k <= m; k *= 2)\r\n"
    "	{\r\n"
    "		ha = a / 2;\r\n"
    "		hb = b / 2;\r\n"
    "		if(((a - ha * 2) != 0) && ((b - hb * 2) != 0))\r\n"
    "		{\r\n"
    "			r += k;\r\n"
    "		}\r\n"
    "		a = ha;\r\n"
    "		b = hb;\r\n"
    "	}\r\n"
    "	\r\n"
    "	return float(r);\r\n"
    "}\r\n";

static const char* s_orFunction =
    "float or(int a, int b)\r\n"
    "{\r\n"
    "	int r = 0;\r\n"
    "	int ha, hb;\r\n"
    "	\r\n"
    "	int m = int(max(float(a), float(b)));\r\n"
    "	\r\n"
    "	for(int k = 1; k <= m; k *= 2)\r\n"
    "	{\r\n"
    "		ha = a / 2;\r\n"
    "		hb = b / 2;\r\n"
    "		if(((a - ha * 2) != 0) || ((b - hb * 2) != 0))\r\n"
    "		{\r\n"
    "			r += k;\r\n"
    "		}\r\n"
    "		a = ha;\r\n"
    "		b = hb;\r\n"
    "	}\r\n"
    "	\r\n"
    "	return float(r);\r\n"
    "}\r\n";

Framework::OpenGl::ProgramPtr CGSH_OpenGL::GenerateShader(const SHADERCAPS& caps)
{
	auto vertexShader = GenerateVertexShader(caps);
	auto fragmentShader = GenerateFragmentShader(caps);

	auto result = std::make_shared<Framework::OpenGl::CProgram>();

	result->AttachShader(vertexShader);
	result->AttachShader(fragmentShader);

	glBindAttribLocation(*result, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION), "a_position");
	glBindAttribLocation(*result, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::DEPTH), "a_depth");
	glBindAttribLocation(*result, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::COLOR), "a_color");
	glBindAttribLocation(*result, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), "a_texCoord");
	glBindAttribLocation(*result, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::FOG), "a_fog");

#ifdef USE_DUALSOURCE_BLENDING
	glBindFragDataLocationIndexed(*result, 0, 0, "fragColor");
	glBindFragDataLocationIndexed(*result, 0, 1, "blendColor");
#endif

	FRAMEWORK_MAYBE_UNUSED bool linkResult = result->Link();
	assert(linkResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGL::GenerateVertexShader(const SHADERCAPS& caps)
{
	std::stringstream shaderBuilder;
	shaderBuilder << GLSL_VERSION << std::endl;

	shaderBuilder << "layout(std140) uniform VertexParams" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	mat4 g_projMatrix;" << std::endl;
	shaderBuilder << "	mat4 g_texMatrix;" << std::endl;
	shaderBuilder << "};" << std::endl;

	shaderBuilder << "in vec2 a_position;" << std::endl;
	shaderBuilder << "in uint a_depth;" << std::endl;
	shaderBuilder << "in vec4 a_color;" << std::endl;
	shaderBuilder << "in vec3 a_texCoord;" << std::endl;

	shaderBuilder << "out float v_depth;" << std::endl;
	shaderBuilder << "out vec4 v_color;" << std::endl;
	shaderBuilder << "out vec3 v_texCoord;" << std::endl;
	if(caps.hasFog)
	{
		shaderBuilder << "in float a_fog;" << std::endl;
		shaderBuilder << "out float v_fog;" << std::endl;
	}

	shaderBuilder << "void main()" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	vec4 texCoord = g_texMatrix * vec4(a_texCoord, 1);" << std::endl;
	shaderBuilder << "	v_depth = float(a_depth) / 4294967296.0;" << std::endl;
	shaderBuilder << "	v_color = a_color;" << std::endl;
	shaderBuilder << "	v_texCoord = texCoord.xyz;" << std::endl;
	if(caps.hasFog)
	{
		shaderBuilder << "	v_fog = a_fog;" << std::endl;
	}
	shaderBuilder << "	gl_Position = g_projMatrix * vec4(a_position, 0, 1);" << std::endl;
	shaderBuilder << "}" << std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_VERTEX_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	FRAMEWORK_MAYBE_UNUSED bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGL::GenerateFragmentShader(const SHADERCAPS& caps)
{
	bool alphaTestCanDiscardDepth = (caps.hasAlphaTest) && ((caps.alphaFailMethod == ALPHA_TEST_FAIL_FBONLY) || (caps.alphaFailMethod == ALPHA_TEST_FAIL_RGBONLY));
	bool useFramebufferFetch = (caps.hasAlphaBlend || caps.hasAlphaTest || caps.hasDestAlphaTest) && m_hasFramebufferFetchExtension;
	bool useFramebufferFetchDepth = alphaTestCanDiscardDepth && m_hasFramebufferFetchDepthExtension;

	std::stringstream shaderBuilder;
	shaderBuilder << GLSL_VERSION << std::endl;

	if(useFramebufferFetch)
	{
		shaderBuilder << "#extension GL_EXT_shader_framebuffer_fetch : require" << std::endl;
	}
	if(useFramebufferFetchDepth)
	{
		shaderBuilder << "#extension GL_ARM_shader_framebuffer_fetch_depth_stencil : require" << std::endl;
	}

	shaderBuilder << "precision mediump float;" << std::endl;

	shaderBuilder << "in highp float v_depth;" << std::endl;
	shaderBuilder << "in vec4 v_color;" << std::endl;
	shaderBuilder << "in highp vec3 v_texCoord;" << std::endl;
	if(caps.hasFog)
	{
		shaderBuilder << "in float v_fog;" << std::endl;
	}

	if(useFramebufferFetch)
	{
		shaderBuilder << "inout vec4 fragColor;" << std::endl;
	}
	else
	{
		shaderBuilder << "out vec4 fragColor;" << std::endl;
	}

#ifdef USE_DUALSOURCE_BLENDING
	shaderBuilder << "out vec4 blendColor;" << std::endl;
#endif

	shaderBuilder << "uniform sampler2D g_texture;" << std::endl;
	shaderBuilder << "uniform sampler2D g_palette;" << std::endl;

	shaderBuilder << "layout(std140) uniform FragmentParams" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	vec2 g_textureSize;" << std::endl;
	shaderBuilder << "	vec2 g_texelSize;" << std::endl;
	shaderBuilder << "	vec2 g_clampMin;" << std::endl;
	shaderBuilder << "	vec2 g_clampMax;" << std::endl;
	shaderBuilder << "	float g_texA0;" << std::endl;
	shaderBuilder << "	float g_texA1;" << std::endl;
	shaderBuilder << "	uint g_alphaRef;" << std::endl;
	shaderBuilder << "	float g_alphaFix;" << std::endl;
	shaderBuilder << "	vec3 g_fogColor;" << std::endl;
	shaderBuilder << "};" << std::endl;

	if(caps.texClampS == TEXTURE_CLAMP_MODE_REGION_REPEAT || caps.texClampT == TEXTURE_CLAMP_MODE_REGION_REPEAT)
	{
		shaderBuilder << s_andFunction << std::endl;
		shaderBuilder << s_orFunction << std::endl;
	}

	shaderBuilder << "float combineColors(float a, float b)" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	uint aInt = uint(a * 255.0);" << std::endl;
	shaderBuilder << "	uint bInt = uint(b * 255.0);" << std::endl;
	shaderBuilder << "	uint result = min((aInt * bInt) >> 7, 255u);" << std::endl;
	shaderBuilder << "	return float(result) / 255.0;" << std::endl;
	shaderBuilder << "}" << std::endl;

	shaderBuilder << "vec4 expandAlpha(vec4 inputColor)" << std::endl;
	shaderBuilder << "{" << std::endl;
	if(caps.texUseAlphaExpansion)
	{
		shaderBuilder << "	float alpha = mix(g_texA0, g_texA1, inputColor.a);" << std::endl;
		if(caps.texBlackIsTransparent)
		{
			shaderBuilder << "	float black = inputColor.r + inputColor.g + inputColor.b;" << std::endl;
			shaderBuilder << "	if(black == 0.0) alpha = 0.0;" << std::endl;
		}
		shaderBuilder << "	return vec4(inputColor.rgb, alpha);" << std::endl;
	}
	else
	{
		shaderBuilder << "	return inputColor;" << std::endl;
	}
	shaderBuilder << "}" << std::endl;

	shaderBuilder << "void main()" << std::endl;
	shaderBuilder << "{" << std::endl;

	if(useFramebufferFetch && caps.hasDestAlphaTest)
	{
		switch(caps.destAlphaTestRef)
		{
		case 0:
			shaderBuilder << "	if(fragColor.a >= 0.5) discard;" << std::endl;
			break;
		case 1:
			shaderBuilder << "	if(fragColor.a < 0.5) discard;" << std::endl;
			break;
		}
	}

	shaderBuilder << "	highp vec3 texCoord = v_texCoord;" << std::endl;
	shaderBuilder << "	texCoord.st /= texCoord.p;" << std::endl;

	if((caps.texClampS != TEXTURE_CLAMP_MODE_STD) || (caps.texClampT != TEXTURE_CLAMP_MODE_STD))
	{
		shaderBuilder << "	texCoord.st *= g_textureSize.st;" << std::endl;
		shaderBuilder << GenerateTexCoordClampingSection(static_cast<TEXTURE_CLAMP_MODE>(caps.texClampS), "s");
		shaderBuilder << GenerateTexCoordClampingSection(static_cast<TEXTURE_CLAMP_MODE>(caps.texClampT), "t");
		shaderBuilder << "	texCoord.st /= g_textureSize.st;" << std::endl;
	}

	shaderBuilder << "	vec4 textureColor = vec4(1, 1, 1, 1);" << std::endl;
	if(caps.isIndexedTextureSource())
	{
		if(!caps.texBilinearFilter)
		{
			shaderBuilder << "	float colorIndex = texture(g_texture, texCoord.st).r * 255.0;" << std::endl;
			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	float paletteTexelBias = 0.5 / 16.0;" << std::endl;
				shaderBuilder << "	textureColor = expandAlpha(texture(g_palette, vec2(colorIndex / 16.0 + paletteTexelBias, 0)));" << std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	float paletteTexelBias = 0.5 / 256.0;" << std::endl;
				shaderBuilder << "	textureColor = expandAlpha(texture(g_palette, vec2(colorIndex / 256.0 + paletteTexelBias, 0)));" << std::endl;
			}
		}
		else
		{
			shaderBuilder << "	float tlIdx = texture(g_texture, texCoord.st                                     ).r * 255.0;" << std::endl;
			shaderBuilder << "	float trIdx = texture(g_texture, texCoord.st + vec2(g_texelSize.x, 0)            ).r * 255.0;" << std::endl;
			shaderBuilder << "	float blIdx = texture(g_texture, texCoord.st + vec2(0, g_texelSize.y)            ).r * 255.0;" << std::endl;
			shaderBuilder << "	float brIdx = texture(g_texture, texCoord.st + vec2(g_texelSize.x, g_texelSize.y)).r * 255.0;" << std::endl;

			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	float paletteTexelBias = 0.5 / 16.0;" << std::endl;
				shaderBuilder << "	vec4 tl = expandAlpha(texture(g_palette, vec2(tlIdx / 16.0 + paletteTexelBias, 0)));" << std::endl;
				shaderBuilder << "	vec4 tr = expandAlpha(texture(g_palette, vec2(trIdx / 16.0 + paletteTexelBias, 0)));" << std::endl;
				shaderBuilder << "	vec4 bl = expandAlpha(texture(g_palette, vec2(blIdx / 16.0 + paletteTexelBias, 0)));" << std::endl;
				shaderBuilder << "	vec4 br = expandAlpha(texture(g_palette, vec2(brIdx / 16.0 + paletteTexelBias, 0)));" << std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	float paletteTexelBias = 0.5 / 256.0;" << std::endl;
				shaderBuilder << "	vec4 tl = expandAlpha(texture(g_palette, vec2(tlIdx / 256.0 + paletteTexelBias, 0)));" << std::endl;
				shaderBuilder << "	vec4 tr = expandAlpha(texture(g_palette, vec2(trIdx / 256.0 + paletteTexelBias, 0)));" << std::endl;
				shaderBuilder << "	vec4 bl = expandAlpha(texture(g_palette, vec2(blIdx / 256.0 + paletteTexelBias, 0)));" << std::endl;
				shaderBuilder << "	vec4 br = expandAlpha(texture(g_palette, vec2(brIdx / 256.0 + paletteTexelBias, 0)));" << std::endl;
			}

			shaderBuilder << "	highp vec2 f = fract(texCoord.st * g_textureSize);" << std::endl;
			shaderBuilder << "	vec4 tA = mix(tl, tr, f.x);" << std::endl;
			shaderBuilder << "	vec4 tB = mix(bl, br, f.x);" << std::endl;
			shaderBuilder << "	textureColor = mix(tA, tB, f.y);" << std::endl;
		}
	}
	else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_STD)
	{
		shaderBuilder << "	textureColor = expandAlpha(texture(g_texture, texCoord.st));" << std::endl;
	}

	if(caps.texSourceMode != TEXTURE_SOURCE_MODE_NONE)
	{
		if(!caps.texHasAlpha)
		{
			shaderBuilder << "	textureColor.a = 1.0;" << std::endl;
		}

		switch(caps.texFunction)
		{
		case TEX0_FUNCTION_MODULATE:
			shaderBuilder << "	textureColor.rgb = clamp(textureColor.rgb * v_color.rgb * 2.0, 0.0, 1.0);" << std::endl;
			if(!caps.texHasAlpha)
			{
				shaderBuilder << "	textureColor.a = v_color.a;" << std::endl;
			}
			else
			{
				shaderBuilder << "	textureColor.a = combineColors(textureColor.a, v_color.a);" << std::endl;
			}
			break;
		case TEX0_FUNCTION_DECAL:
			if(!caps.texHasAlpha)
			{
				shaderBuilder << "	textureColor.a = v_color.a;" << std::endl;
			}
			break;
		case TEX0_FUNCTION_HIGHLIGHT:
			shaderBuilder << "	textureColor.rgb = clamp(textureColor.rgb * v_color.rgb * 2.0, 0.0, 1.0) + v_color.aaa;" << std::endl;
			if(!caps.texHasAlpha)
			{
				shaderBuilder << "	textureColor.a = v_color.a;" << std::endl;
			}
			else
			{
				shaderBuilder << "	textureColor.a += v_color.a;" << std::endl;
			}
			break;
		case TEX0_FUNCTION_HIGHLIGHT2:
			shaderBuilder << "	textureColor.rgb = clamp(textureColor.rgb * v_color.rgb * 2.0, 0.0, 1.0) + v_color.aaa;" << std::endl;
			if(!caps.texHasAlpha)
			{
				shaderBuilder << "	textureColor.a = v_color.a;" << std::endl;
			}
			break;
		default:
			assert(0);
			break;
		}
	}
	else
	{
		shaderBuilder << "	textureColor = v_color;" << std::endl;
	}

	if(useFramebufferFetch)
	{
		shaderBuilder << "	bool outputColor = true;" << std::endl;
		shaderBuilder << "	bool outputAlpha = true;" << std::endl;
	}
	if(useFramebufferFetchDepth)
	{
		shaderBuilder << "	bool outputDepth = true;" << std::endl;
	}

	if(caps.hasAlphaTest)
	{
		shaderBuilder << GenerateAlphaTestSection(static_cast<ALPHA_TEST_METHOD>(caps.alphaTestMethod), static_cast<ALPHA_TEST_FAIL_METHOD>(caps.alphaFailMethod));
	}

	// ----------------------
	// Prepare final color and alpha values before blending and writing

	if(caps.hasFog)
	{
		shaderBuilder << "	vec3 finalColor = mix(textureColor.rgb, g_fogColor, v_fog);" << std::endl;
	}
	else
	{
		shaderBuilder << "	vec3 finalColor = textureColor.xyz;" << std::endl;
	}

	//For proper alpha blending, alpha has to be multiplied by 2 (0x80 -> 1.0)
#ifdef USE_DUALSOURCE_BLENDING
	shaderBuilder << "	float finalAlpha = textureColor.a;" << std::endl;
	shaderBuilder << "	blendColor.a = clamp(textureColor.a * 2.0, 0.0, 1.0);" << std::endl;
#else
	if(useFramebufferFetch)
	{
		shaderBuilder << "	float finalAlpha = textureColor.a;" << std::endl;
	}
	else
	{
		//This has the side effect of not writing a proper value in the framebuffer (should write alpha "as is")
		shaderBuilder << "	float finalAlpha = clamp(textureColor.a * 2.0, 0.0, 1.0);" << std::endl;
	}
#endif

	if(useFramebufferFetch)
	{
		if(caps.hasAlphaBlend)
		{
			shaderBuilder << GenerateAlphaBlendSection(
			    static_cast<ALPHABLEND_ABD>(caps.alphaBlendA), static_cast<ALPHABLEND_ABD>(caps.alphaBlendB),
			    static_cast<ALPHABLEND_C>(caps.alphaBlendC), static_cast<ALPHABLEND_ABD>(caps.alphaBlendD));
		}

		shaderBuilder << "	if(outputColor)" << std::endl;
		shaderBuilder << "	{" << std::endl;
		shaderBuilder << "		fragColor.xyz = finalColor;" << std::endl;
		shaderBuilder << "	}" << std::endl;

		shaderBuilder << "	if(outputAlpha)" << std::endl;
		shaderBuilder << "	{" << std::endl;
		shaderBuilder << "		fragColor.a = finalAlpha;" << std::endl;
		shaderBuilder << "	}" << std::endl;
	}
	else
	{
		shaderBuilder << "	fragColor.xyz = finalColor;" << std::endl;
		shaderBuilder << "	fragColor.a = finalAlpha;" << std::endl;

		if(caps.colorOutputWhite)
		{
			shaderBuilder << "	fragColor.xyz = vec3(1, 1, 1);" << std::endl;
		}
	}

	// ----------------------

	if(useFramebufferFetchDepth)
	{
		shaderBuilder << "	if(outputDepth)" << std::endl;
		shaderBuilder << "	{" << std::endl;
		shaderBuilder << "		gl_FragDepth = v_depth;" << std::endl;
		shaderBuilder << "	}" << std::endl;
		shaderBuilder << "	else" << std::endl;
		shaderBuilder << "	{" << std::endl;
		shaderBuilder << "		highp float dstDepth = gl_LastFragDepthARM;" << std::endl;
		if(caps.alphaTestDepthTest_DepthFetch)
		{
			if(caps.alphaTestDepthTestEqual_DepthFetch)
			{
				shaderBuilder << "		if(!(v_depth >= dstDepth)) discard;" << std::endl;
			}
			else
			{
				shaderBuilder << "		if(!(v_depth > dstDepth)) discard;" << std::endl;
			}
		}
		shaderBuilder << "		gl_FragDepth = dstDepth;" << std::endl;
		shaderBuilder << "	}" << std::endl;
	}
	else
	{
		shaderBuilder << "	gl_FragDepth = v_depth;" << std::endl;
	}

	// ----------------------

	shaderBuilder << "}" << std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_FRAGMENT_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	FRAMEWORK_MAYBE_UNUSED bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

	return result;
}

std::string CGSH_OpenGL::GenerateTexCoordClampingSection(TEXTURE_CLAMP_MODE clampMode, const char* coordinate)
{
	std::stringstream shaderBuilder;

	switch(clampMode)
	{
	case TEXTURE_CLAMP_MODE_STD:
		//Using the GPU's built-in clamping, nothing to do.
		break;
	case TEXTURE_CLAMP_MODE_CLAMP:
		shaderBuilder << "	texCoord." << coordinate << " = clamp(texCoord." << coordinate
		              << ", g_clampMin." << coordinate << ", g_clampMax." << coordinate << ");" << std::endl;
		break;
	case TEXTURE_CLAMP_MODE_REGION_CLAMP:
		shaderBuilder << "	texCoord." << coordinate << " = min(g_clampMax." << coordinate << ", "
		              << "max(g_clampMin." << coordinate << ", texCoord." << coordinate << "));" << std::endl;
		break;
	case TEXTURE_CLAMP_MODE_REGION_REPEAT:
		shaderBuilder << "	texCoord." << coordinate << " = or(int(and(int(texCoord." << coordinate << "), "
		              << "int(g_clampMin." << coordinate << "))), int(g_clampMax." << coordinate << "));";
		break;
	case TEXTURE_CLAMP_MODE_REGION_REPEAT_SIMPLE:
		shaderBuilder << "	texCoord." << coordinate << " = mod(texCoord." << coordinate << ", "
		              << "g_clampMin." << coordinate << ") + g_clampMax." << coordinate << ";" << std::endl;
		break;
	default:
		assert(false);
		break;
	}

	std::string shaderSource = shaderBuilder.str();
	return shaderSource;
}

std::string CGSH_OpenGL::GenerateAlphaTestSection(ALPHA_TEST_METHOD testMethod, ALPHA_TEST_FAIL_METHOD failMethod)
{
	std::stringstream shaderBuilder;

	const char* test = "	if(false)";

	//testMethod is the condition to pass the test
	switch(testMethod)
	{
	case ALPHA_TEST_NEVER:
		test = "	if(true)";
		break;
	case ALPHA_TEST_ALWAYS:
		test = "	if(false)";
		break;
	case ALPHA_TEST_LESS:
		test = "	if(textureColorAlphaInt >= g_alphaRef)";
		break;
	case ALPHA_TEST_LEQUAL:
		test = "	if(textureColorAlphaInt > g_alphaRef)";
		break;
	case ALPHA_TEST_EQUAL:
		test = "	if(textureColorAlphaInt != g_alphaRef)";
		break;
	case ALPHA_TEST_GEQUAL:
		test = "	if(textureColorAlphaInt < g_alphaRef)";
		break;
	case ALPHA_TEST_GREATER:
		test = "	if(textureColorAlphaInt <= g_alphaRef)";
		break;
	case ALPHA_TEST_NOTEQUAL:
		test = "	if(textureColorAlphaInt == g_alphaRef)";
		break;
	default:
		assert(false);
		break;
	}

	shaderBuilder << "	uint textureColorAlphaInt = uint(textureColor.a * 255.0);" << std::endl;
	shaderBuilder << test << std::endl;
	shaderBuilder << "	{" << std::endl;

	switch(failMethod)
	{
	case ALPHA_TEST_FAIL_KEEP:
		// No write at all
		shaderBuilder << "		discard;" << std::endl;
		break;
	case ALPHA_TEST_FAIL_FBONLY:
		// Only write color and alpha
		// TODO: We cannot prevent depth from being written at the moment
		// Failure note: We rather accept writing depth here, than discarding the
		// whole pixel, as most games work better with this hack.
		// Breaks foliage in some games (Grandia X/3, Naruto, etc.)
		// If we have depth buffer fetch, we can properly handle it.
		if(m_hasFramebufferFetchDepthExtension)
		{
			shaderBuilder << "		outputDepth = false;" << std::endl;
		}
		break;
	case ALPHA_TEST_FAIL_ZBONLY:
		// Only write depth
		if(m_hasFramebufferFetchExtension)
		{
			shaderBuilder << "		outputColor = false;" << std::endl;
			shaderBuilder << "		outputAlpha = false;" << std::endl;
		}
		else
		{
			// TODO: Prevent framebuffer writing without extension
			shaderBuilder << "		discard;" << std::endl;
			// Failure note: This also discards depth
		}
		break;
	case ALPHA_TEST_FAIL_RGBONLY:
		// Only write color
		if(m_hasFramebufferFetchExtension)
		{
			shaderBuilder << "		outputAlpha = false;" << std::endl;
		}
		if(m_hasFramebufferFetchDepthExtension)
		{
			shaderBuilder << "		outputDepth = false;" << std::endl;
		}
		// TODO: Prevent alpha and depth writing without extension
		// Failure note: We are somewhat out of luck, and just draw the pixel as is.
		// This is completely wrong, but nothing we can do about it at this point.
		break;
	}

	shaderBuilder << "	}" << std::endl;

	auto shaderSource = shaderBuilder.str();
	return shaderSource;
}

std::string CGSH_OpenGL::GenerateAlphaBlendSection(ALPHABLEND_ABD factorA, ALPHABLEND_ABD factorB,
                                                   ALPHABLEND_C factorC, ALPHABLEND_ABD factorD)
{
	static const auto getABDParam =
	    [](unsigned int abd) {
		    switch(abd)
		    {
		    default:
			    assert(false);
			    [[fallthrough]];
		    case ALPHABLEND_ABD_CS:
			    return "finalColor";
		    case ALPHABLEND_ABD_CD:
			    return "fragColor.xyz";
		    case ALPHABLEND_ABD_ZERO:
			    return "vec3(0, 0, 0)";
		    }
	    };
	static const auto getCParam =
	    [](unsigned int abd) {
		    switch(abd)
		    {
		    default:
			    assert(false);
			    [[fallthrough]];
		    case ALPHABLEND_C_AS:
			    return "finalAlpha";
		    case ALPHABLEND_C_AD:
			    return "fragColor.a";
		    case ALPHABLEND_C_FIX:
			    return "g_alphaFix";
		    }
	    };

	auto valueA = getABDParam(factorA);
	auto valueB = getABDParam(factorB);
	auto valueC = getCParam(factorC);
	auto valueD = getABDParam(factorD);

	std::stringstream shaderBuilder;
	shaderBuilder << "	finalColor = (" << valueA << " - " << valueB << ") * (" << valueC << " * 2.0) + " << valueD << ";" << std::endl;

	auto shaderSource = shaderBuilder.str();
	return shaderSource;
}

Framework::OpenGl::ProgramPtr CGSH_OpenGL::GeneratePresentProgram()
{
	Framework::OpenGl::CShader vertexShader(GL_VERTEX_SHADER);
	Framework::OpenGl::CShader pixelShader(GL_FRAGMENT_SHADER);

	{
		std::stringstream shaderBuilder;
		shaderBuilder << GLSL_VERSION << std::endl;
		shaderBuilder << "in vec2 a_position;" << std::endl;
		shaderBuilder << "in vec2 a_texCoord;" << std::endl;
		shaderBuilder << "out vec2 v_texCoord;" << std::endl;
		shaderBuilder << "uniform vec2 g_texCoordScale;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	v_texCoord = a_texCoord * g_texCoordScale;" << std::endl;
		shaderBuilder << "	gl_Position = vec4(a_position, 0, 1);" << std::endl;
		shaderBuilder << "}" << std::endl;

		vertexShader.SetSource(shaderBuilder.str().c_str());
		FRAMEWORK_MAYBE_UNUSED bool result = vertexShader.Compile();
		assert(result);
	}

	{
		std::stringstream shaderBuilder;
		shaderBuilder << GLSL_VERSION << std::endl;
		shaderBuilder << "precision mediump float;" << std::endl;
		shaderBuilder << "in vec2 v_texCoord;" << std::endl;
		shaderBuilder << "out vec4 fragColor;" << std::endl;
		shaderBuilder << "uniform sampler2D g_texture;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	fragColor = texture(g_texture, v_texCoord);" << std::endl;
		shaderBuilder << "}" << std::endl;

		pixelShader.SetSource(shaderBuilder.str().c_str());
		FRAMEWORK_MAYBE_UNUSED bool result = pixelShader.Compile();
		assert(result);
	}

	auto program = std::make_shared<Framework::OpenGl::CProgram>();

	{
		program->AttachShader(vertexShader);
		program->AttachShader(pixelShader);

		glBindAttribLocation(*program, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION), "a_position");
		glBindAttribLocation(*program, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), "a_texCoord");

		FRAMEWORK_MAYBE_UNUSED bool result = program->Link();
		assert(result);
	}

	return program;
}

Framework::OpenGl::ProgramPtr CGSH_OpenGL::GenerateCopyToFbProgram()
{
	Framework::OpenGl::CShader vertexShader(GL_VERTEX_SHADER);
	Framework::OpenGl::CShader pixelShader(GL_FRAGMENT_SHADER);

	{
		std::stringstream shaderBuilder;
		shaderBuilder << GLSL_VERSION << std::endl;
		shaderBuilder << "in vec2 a_position;" << std::endl;
		shaderBuilder << "in vec2 a_texCoord;" << std::endl;
		shaderBuilder << "out vec2 v_texCoord;" << std::endl;
		shaderBuilder << "uniform vec2 g_srcPosition;" << std::endl;
		shaderBuilder << "uniform vec2 g_srcSize;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	v_texCoord = (a_texCoord * g_srcSize) + g_srcPosition;" << std::endl;
		shaderBuilder << "	gl_Position = vec4(a_position, 0, 1);" << std::endl;
		shaderBuilder << "}" << std::endl;

		vertexShader.SetSource(shaderBuilder.str().c_str());
		FRAMEWORK_MAYBE_UNUSED bool result = vertexShader.Compile();
		assert(result);
	}

	{
		std::stringstream shaderBuilder;
		shaderBuilder << GLSL_VERSION << std::endl;
		shaderBuilder << "precision mediump float;" << std::endl;
		shaderBuilder << "in vec2 v_texCoord;" << std::endl;
		shaderBuilder << "out vec4 fragColor;" << std::endl;
		shaderBuilder << "uniform sampler2D g_texture;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	fragColor = texture(g_texture, v_texCoord);" << std::endl;
		shaderBuilder << "}" << std::endl;

		pixelShader.SetSource(shaderBuilder.str().c_str());
		FRAMEWORK_MAYBE_UNUSED bool result = pixelShader.Compile();
		assert(result);
	}

	auto program = std::make_shared<Framework::OpenGl::CProgram>();

	{
		program->AttachShader(vertexShader);
		program->AttachShader(pixelShader);

		glBindAttribLocation(*program, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::POSITION), "a_position");
		glBindAttribLocation(*program, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), "a_texCoord");

		FRAMEWORK_MAYBE_UNUSED bool result = program->Link();
		assert(result);
	}

	return program;
}
