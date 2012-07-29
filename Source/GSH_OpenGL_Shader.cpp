#include "GSH_OpenGL.h"
#include <assert.h>

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

	bool linkResult = result->Link();
	assert(linkResult);

	assert(glGetError() == GL_NO_ERROR);

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGL::GenerateVertexShader(const SHADERCAPS& caps)
{
	std::stringstream shaderBuilder;
	shaderBuilder << "void main()"								<< std::endl;
	shaderBuilder << "{"										<< std::endl;
	shaderBuilder << "	gl_TexCoord[0] = gl_MultiTexCoord0;"	<< std::endl;
	shaderBuilder << "	gl_FrontColor = gl_Color;"				<< std::endl;
	shaderBuilder << "	gl_Position = ftransform();"			<< std::endl;
	shaderBuilder << "	gl_FogFragCoord = gl_FogCoord;"			<< std::endl;
	shaderBuilder << "}"										<< std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_VERTEX_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	bool compilationResult = result.Compile();
	assert(compilationResult);

	assert(glGetError() == GL_NO_ERROR);

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGL::GenerateFragmentShader(const SHADERCAPS& caps)
{
	std::stringstream shaderBuilder;

	//Unused uniforms will get optimized away
	shaderBuilder << "uniform sampler2D g_texture;"															<< std::endl;
	shaderBuilder << "uniform sampler2D g_palette;"															<< std::endl;
	shaderBuilder << "uniform vec2 g_textureSize;"															<< std::endl;
	shaderBuilder << "uniform vec2 g_texelSize;"															<< std::endl;
	shaderBuilder << "uniform vec2 g_clampMin;"																<< std::endl;
	shaderBuilder << "uniform vec2 g_clampMax;"																<< std::endl;

	if(caps.texClampS == TEXTURE_CLAMP_MODE_REGION_REPEAT || caps.texClampT == TEXTURE_CLAMP_MODE_REGION_REPEAT)
	{
		shaderBuilder << s_andFunction << std::endl;
		shaderBuilder << s_orFunction << std::endl;
	}

	shaderBuilder << "void main()"																			<< std::endl;
	shaderBuilder << "{"																					<< std::endl;
	shaderBuilder << "	vec4 texCoord = gl_TexCoord[0];"													<< std::endl;
	shaderBuilder << "	texCoord.st /= texCoord.q;"															<< std::endl;

	if((caps.texClampS != TEXTURE_CLAMP_MODE_STD) || (caps.texClampT != TEXTURE_CLAMP_MODE_STD))
	{
		shaderBuilder << "	texCoord.st *= g_textureSize.st;"													<< std::endl;
		shaderBuilder << GenerateTexCoordClampingSection(static_cast<TEXTURE_CLAMP_MODE>(caps.texClampS), "s");
		shaderBuilder << GenerateTexCoordClampingSection(static_cast<TEXTURE_CLAMP_MODE>(caps.texClampT), "t");
		shaderBuilder << "	texCoord.st /= g_textureSize.st;"													<< std::endl;
	}

	shaderBuilder << "	vec4 textureColor = vec4(1, 1, 1, 1);"												<< std::endl;
	if(caps.isIndexedTextureSource())
	{
		if(!caps.texBilinearFilter)
		{
			shaderBuilder << "	float colorIndex = texture2D(g_texture, texCoord.st).a * 255.0;"				<< std::endl;
			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	textureColor = texture2D(g_palette, vec2(colorIndex / 16.0, 1));"			<< std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	textureColor = texture2D(g_palette, vec2(colorIndex / 256.0, 1));"			<< std::endl;
			}
		}
		else
		{
			shaderBuilder << "	float tlIdx = texture2D(g_texture, texCoord.st                                     ).a * 255.0;"	<< std::endl;
			shaderBuilder << "	float trIdx = texture2D(g_texture, texCoord.st + vec2(g_texelSize.x, 0)            ).a * 255.0;"	<< std::endl;
			shaderBuilder << "	float blIdx = texture2D(g_texture, texCoord.st + vec2(0, g_texelSize.y)            ).a * 255.0;"	<< std::endl;
			shaderBuilder << "	float brIdx = texture2D(g_texture, texCoord.st + vec2(g_texelSize.x, g_texelSize.y)).a * 255.0;"	<< std::endl;

			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	vec4 tl = texture2D(g_palette, vec2(tlIdx / 16.0, 1));"									<< std::endl;
				shaderBuilder << "	vec4 tr = texture2D(g_palette, vec2(trIdx / 16.0, 1));"									<< std::endl;
				shaderBuilder << "	vec4 bl = texture2D(g_palette, vec2(blIdx / 16.0, 1));"									<< std::endl;
				shaderBuilder << "	vec4 br = texture2D(g_palette, vec2(brIdx / 16.0, 1));"									<< std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	vec4 tl = texture2D(g_palette, vec2(tlIdx / 256.0, 1));"								<< std::endl;
				shaderBuilder << "	vec4 tr = texture2D(g_palette, vec2(trIdx / 256.0, 1));"								<< std::endl;
				shaderBuilder << "	vec4 bl = texture2D(g_palette, vec2(blIdx / 256.0, 1));"								<< std::endl;
				shaderBuilder << "	vec4 br = texture2D(g_palette, vec2(brIdx / 256.0, 1));"								<< std::endl;
			}

			shaderBuilder << "	vec2 f = fract(texCoord.xy * g_textureSize);"												<< std::endl;
			shaderBuilder << "	vec4 tA = mix(tl, tr, f.x);"																<< std::endl;
			shaderBuilder << "	vec4 tB = mix(bl, br, f.x);"																<< std::endl;
			shaderBuilder << "	textureColor = mix(tA, tB, f.y);"															<< std::endl;
		}
	}
	else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_STD)
	{
		shaderBuilder << "	textureColor = texture2D(g_texture, texCoord.st);"								<< std::endl;
	}

	switch(caps.texFunction)
	{
	default:
		assert(0);
	case TEX0_FUNCTION_MODULATE:
		shaderBuilder << "	textureColor *= gl_Color;"														<< std::endl;
		break;
	case TEX0_FUNCTION_DECAL:
		break;
	}

	if(caps.hasFog)
	{
		shaderBuilder << "	gl_FragColor = vec4(mix(textureColor.rgb, gl_Fog.color.rgb, gl_FogFragCoord), textureColor.a);" << std::endl;
	}
	else
	{
		shaderBuilder << "	gl_FragColor = textureColor;"																	<< std::endl;
	}

	shaderBuilder << "}"																					<< std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_FRAGMENT_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	bool compilationResult = result.Compile();
	assert(compilationResult);

	assert(glGetError() == GL_NO_ERROR);

	return result;
}

std::string CGSH_OpenGL::GenerateTexCoordClampingSection(TEXTURE_CLAMP_MODE clampMode, const char* coordinate)
{
	std::stringstream shaderBuilder;

	switch(clampMode)
	{
	case TEXTURE_CLAMP_MODE_REGION_CLAMP:
		shaderBuilder << "	texCoord." << coordinate << " = min(g_clampMax." << coordinate << ", " <<
			"max(g_clampMin." << coordinate << ", texCoord." << coordinate << "));" << std::endl;
		break;
	case TEXTURE_CLAMP_MODE_REGION_REPEAT:
		shaderBuilder << "	texCoord." << coordinate << " = or(int(and(int(texCoord." << coordinate << "), " << 
			"int(g_clampMin." << coordinate << "))), int(g_clampMax." << coordinate << "));";
		break;
	case TEXTURE_CLAMP_MODE_REGION_REPEAT_SIMPLE:
		shaderBuilder << "	texCoord." << coordinate << " = mod(texCoord." << coordinate << ", " << 
			"g_clampMin." << coordinate << ") + g_clampMax." << coordinate << ";" << std::endl;
		break;
	}

	std::string shaderSource = shaderBuilder.str();
	return shaderSource;
}
