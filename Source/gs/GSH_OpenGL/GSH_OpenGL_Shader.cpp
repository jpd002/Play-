#include "GSH_OpenGL.h"
#include <assert.h>
#include <sstream>

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
	glBindAttribLocation(*result, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::COLOR), "a_color");
	glBindAttribLocation(*result, static_cast<GLuint>(PRIM_VERTEX_ATTRIB::TEXCOORD), "a_texCoord");

	bool linkResult = result->Link();
	assert(linkResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGL::GenerateVertexShader(const SHADERCAPS& caps)
{
	std::stringstream shaderBuilder;
#ifndef GLES_COMPATIBILITY
	shaderBuilder << "void main()"													<< std::endl;
	shaderBuilder << "{"															<< std::endl;
	shaderBuilder << "	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"	<< std::endl;
	shaderBuilder << "	gl_FrontColor = gl_Color;"									<< std::endl;
	shaderBuilder << "	gl_Position = ftransform();"								<< std::endl;
	shaderBuilder << "	gl_FogFragCoord = gl_FogCoord;"								<< std::endl;
	shaderBuilder << "}"															<< std::endl;
#else
	shaderBuilder << "#version 300 es" << std::endl;

	shaderBuilder << "uniform mat4 g_projMatrix;" << std::endl;
	shaderBuilder << "uniform mat4 g_texMatrix;" << std::endl;

	shaderBuilder << "in vec3 a_position;" << std::endl;
	shaderBuilder << "in vec4 a_color;" << std::endl;
	shaderBuilder << "in vec3 a_texCoord;" << std::endl;

	shaderBuilder << "out vec4 v_color;" << std::endl;
	shaderBuilder << "out vec3 v_texCoord;" << std::endl;

	shaderBuilder << "void main()" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	vec4 texCoord = g_texMatrix * vec4(a_texCoord, 1);" << std::endl;
	shaderBuilder << "	v_color = a_color;" << std::endl;
	shaderBuilder << "	v_texCoord = texCoord.xyz;" << std::endl;
	shaderBuilder << "	gl_Position = g_projMatrix * vec4(a_position, 1);" << std::endl;
	shaderBuilder << "}" << std::endl;
#endif

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_VERTEX_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGL::GenerateFragmentShader(const SHADERCAPS& caps)
{
	std::stringstream shaderBuilder;

#ifndef GLES_COMPATIBILITY
	//Unused uniforms will get optimized away
	shaderBuilder << "uniform sampler2D g_texture;" << std::endl;
	shaderBuilder << "uniform sampler2D g_palette;" << std::endl;
	shaderBuilder << "uniform vec2 g_textureSize;" << std::endl;
	shaderBuilder << "uniform vec2 g_texelSize;" << std::endl;
	shaderBuilder << "uniform vec2 g_clampMin;" << std::endl;
	shaderBuilder << "uniform vec2 g_clampMax;" << std::endl;
	shaderBuilder << "uniform float g_texA0;" << std::endl;
	shaderBuilder << "uniform float g_texA1;" << std::endl;

	if(caps.texClampS == TEXTURE_CLAMP_MODE_REGION_REPEAT || caps.texClampT == TEXTURE_CLAMP_MODE_REGION_REPEAT)
	{
		shaderBuilder << s_andFunction << std::endl;
		shaderBuilder << s_orFunction << std::endl;
	}

	shaderBuilder << "vec4 expandAlpha(vec4 inputColor)" << std::endl;
	shaderBuilder << "{" << std::endl;
	if(caps.texUseAlphaExpansion)
	{
		shaderBuilder << "	float alpha = mix(g_texA0, g_texA1, inputColor.a);" << std::endl;
		if(caps.texBlackIsTransparent)
		{
			shaderBuilder << "	float black = inputColor.r + inputColor.g + inputColor.b;" << std::endl;
			shaderBuilder << "	if(black == 0) alpha = 0;" << std::endl;
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
	shaderBuilder << "	vec4 texCoord = gl_TexCoord[0];" << std::endl;
	shaderBuilder << "	texCoord.st /= texCoord.q;" << std::endl;

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
			shaderBuilder << "	float colorIndex = texture2D(g_texture, texCoord.st).a * 255.0;" << std::endl;
			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	textureColor = expandAlpha(texture2D(g_palette, vec2(colorIndex / 16.0, 1)));" << std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	textureColor = expandAlpha(texture2D(g_palette, vec2(colorIndex / 256.0, 1)));" << std::endl;
			}
		}
		else
		{
			shaderBuilder << "	float tlIdx = texture2D(g_texture, texCoord.st                                     ).a * 255.0;" << std::endl;
			shaderBuilder << "	float trIdx = texture2D(g_texture, texCoord.st + vec2(g_texelSize.x, 0)            ).a * 255.0;" << std::endl;
			shaderBuilder << "	float blIdx = texture2D(g_texture, texCoord.st + vec2(0, g_texelSize.y)            ).a * 255.0;" << std::endl;
			shaderBuilder << "	float brIdx = texture2D(g_texture, texCoord.st + vec2(g_texelSize.x, g_texelSize.y)).a * 255.0;" << std::endl;

			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	vec4 tl = expandAlpha(texture2D(g_palette, vec2(tlIdx / 16.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 tr = expandAlpha(texture2D(g_palette, vec2(trIdx / 16.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 bl = expandAlpha(texture2D(g_palette, vec2(blIdx / 16.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 br = expandAlpha(texture2D(g_palette, vec2(brIdx / 16.0, 1)));" << std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	vec4 tl = expandAlpha(texture2D(g_palette, vec2(tlIdx / 256.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 tr = expandAlpha(texture2D(g_palette, vec2(trIdx / 256.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 bl = expandAlpha(texture2D(g_palette, vec2(blIdx / 256.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 br = expandAlpha(texture2D(g_palette, vec2(brIdx / 256.0, 1)));" << std::endl;
			}

			shaderBuilder << "	vec2 f = fract(texCoord.xy * g_textureSize);" << std::endl;
			shaderBuilder << "	vec4 tA = mix(tl, tr, f.x);" << std::endl;
			shaderBuilder << "	vec4 tB = mix(bl, br, f.x);" << std::endl;
			shaderBuilder << "	textureColor = mix(tA, tB, f.y);" << std::endl;
		}
	}
	else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_STD)
	{
		shaderBuilder << "	textureColor = expandAlpha(texture2D(g_texture, texCoord.st));" << std::endl;
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
			shaderBuilder << "	textureColor *= gl_Color;" << std::endl;
			break;
		case TEX0_FUNCTION_DECAL:
			break;
		case TEX0_FUNCTION_HIGHLIGHT:
			shaderBuilder << "	textureColor.rgb = (textureColor.rgb * gl_Color.rgb) + gl_Color.aaa;" << std::endl;
			if(!caps.texHasAlpha)
			{
				shaderBuilder << "	textureColor.a = gl_Color.a;" << std::endl;
			}
			else
			{
				shaderBuilder << "	textureColor.a += gl_Color.a;" << std::endl;
			}
			break;
		case TEX0_FUNCTION_HIGHLIGHT2:
			shaderBuilder << "	textureColor.rgb = (textureColor.rgb * gl_Color.rgb) + gl_Color.aaa;" << std::endl;
			if(!caps.texHasAlpha)
			{
				shaderBuilder << "	textureColor.a = gl_Color.a;" << std::endl;
			}
			break;
		default:
			assert(0);
			break;
		}
	}
	else
	{
		shaderBuilder << "	textureColor = gl_Color;" << std::endl;
	}

	if(caps.hasFog)
	{
		shaderBuilder << "	gl_FragColor = vec4(mix(textureColor.rgb, gl_Fog.color.rgb, gl_FogFragCoord), textureColor.a);" << std::endl;
	}
	else
	{
		shaderBuilder << "	gl_FragColor = textureColor;" << std::endl;
	}

	shaderBuilder << "}" << std::endl;
#else
	shaderBuilder << "#version 300 es" << std::endl;

	shaderBuilder << "precision mediump float;" << std::endl;

	shaderBuilder << "in vec4 v_color;" << std::endl;
	shaderBuilder << "in vec3 v_texCoord;" << std::endl;
	shaderBuilder << "out vec4 fragColor;" << std::endl;

	shaderBuilder << "uniform sampler2D g_texture;" << std::endl;
	shaderBuilder << "uniform sampler2D g_palette;" << std::endl;
	shaderBuilder << "uniform vec2 g_textureSize;" << std::endl;
	shaderBuilder << "uniform vec2 g_texelSize;" << std::endl;
	shaderBuilder << "uniform float g_texA0;" << std::endl;
	shaderBuilder << "uniform float g_texA1;" << std::endl;

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
	shaderBuilder << "	vec4 textureColor = vec4(1, 1, 1, 1);" << std::endl;
	if(caps.isIndexedTextureSource())
	{
		if(!caps.texBilinearFilter)
		{
			shaderBuilder << "	float colorIndex = textureProj(g_texture, v_texCoord).r * 255.0;" << std::endl;
			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	textureColor = expandAlpha(texture(g_palette, vec2(colorIndex / 15.0, 1)));" << std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	textureColor = expandAlpha(texture(g_palette, vec2(colorIndex / 255.0, 1)));" << std::endl;
			}
		}
		else
		{
			shaderBuilder << "	vec2 projTexCoord = v_texCoord.xy / v_texCoord.z;" << std::endl;
			shaderBuilder << "	float tlIdx = texture(g_texture, projTexCoord                                        ).r * 255.0;" << std::endl;
			shaderBuilder << "	float trIdx = texture(g_texture, projTexCoord + vec2(g_texelSize.x, 0)            ).r * 255.0;" << std::endl;
			shaderBuilder << "	float blIdx = texture(g_texture, projTexCoord + vec2(0, g_texelSize.y)            ).r * 255.0;" << std::endl;
			shaderBuilder << "	float brIdx = texture(g_texture, projTexCoord + vec2(g_texelSize.x, g_texelSize.y)).r * 255.0;" << std::endl;

			if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX4)
			{
				shaderBuilder << "	vec4 tl = expandAlpha(texture(g_palette, vec2(tlIdx / 15.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 tr = expandAlpha(texture(g_palette, vec2(trIdx / 15.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 bl = expandAlpha(texture(g_palette, vec2(blIdx / 15.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 br = expandAlpha(texture(g_palette, vec2(brIdx / 15.0, 1)));" << std::endl;
			}
			else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_IDX8)
			{
				shaderBuilder << "	vec4 tl = expandAlpha(texture(g_palette, vec2(tlIdx / 255.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 tr = expandAlpha(texture(g_palette, vec2(trIdx / 255.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 bl = expandAlpha(texture(g_palette, vec2(blIdx / 255.0, 1)));" << std::endl;
				shaderBuilder << "	vec4 br = expandAlpha(texture(g_palette, vec2(brIdx / 255.0, 1)));" << std::endl;
			}

			shaderBuilder << "	vec2 f = fract(projTexCoord * g_textureSize);" << std::endl;
			shaderBuilder << "	vec4 tA = mix(tl, tr, f.x);" << std::endl;
			shaderBuilder << "	vec4 tB = mix(bl, br, f.x);" << std::endl;
			shaderBuilder << "	textureColor = mix(tA, tB, f.y);" << std::endl;
		}
	}
	else if(caps.texSourceMode == TEXTURE_SOURCE_MODE_STD)
	{
		shaderBuilder << "	textureColor = expandAlpha(textureProj(g_texture, v_texCoord));" << std::endl;
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
			shaderBuilder << "	textureColor *= v_color;" << std::endl;
			break;
		case TEX0_FUNCTION_DECAL:
			break;
		case TEX0_FUNCTION_HIGHLIGHT:
			shaderBuilder << "	textureColor.rgb = (textureColor.rgb * v_color.rgb) + v_color.aaa;" << std::endl;
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
			shaderBuilder << "	textureColor.rgb = (textureColor.rgb * v_color.rgb) + v_color.aaa;" << std::endl;
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
	shaderBuilder << "	fragColor = textureColor;" << std::endl;
	shaderBuilder << "}" << std::endl;
#endif

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_FRAGMENT_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

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

Framework::OpenGl::ProgramPtr CGSH_OpenGL::GeneratePresentProgram()
{
	Framework::OpenGl::CShader vertexShader(GL_VERTEX_SHADER);
	Framework::OpenGl::CShader pixelShader(GL_FRAGMENT_SHADER);

	{
		std::stringstream shaderBuilder;
		shaderBuilder << "#version 300 es" << std::endl;
		shaderBuilder << "in vec3 a_position;" << std::endl;
		shaderBuilder << "out vec2 v_texCoord;" << std::endl;
		shaderBuilder << "uniform vec2 g_texCoordScale;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	vec2 outputPosition;" << std::endl;
		shaderBuilder << "	outputPosition.x = float(gl_VertexID / 2) * 4.0 - 1.0;" << std::endl;
		shaderBuilder << "	outputPosition.y = float(gl_VertexID % 2) * 4.0 - 1.0;" << std::endl;
		shaderBuilder << "	v_texCoord.x = float(gl_VertexID / 2) * 2.0;" << std::endl;
		shaderBuilder << "	v_texCoord.y = 1.0 - float(gl_VertexID % 2) * 2.0;" << std::endl;
		shaderBuilder << "	v_texCoord = v_texCoord * g_texCoordScale;" << std::endl;
		shaderBuilder << "	gl_Position = vec4(outputPosition, 0, 1);" << std::endl;
		shaderBuilder << "}" << std::endl;

		vertexShader.SetSource(shaderBuilder.str().c_str());
		bool result = vertexShader.Compile();
		assert(result);
	}

	{
		std::stringstream shaderBuilder;
		shaderBuilder << "#version 300 es" << std::endl;
		shaderBuilder << "precision mediump float;" << std::endl;
		shaderBuilder << "in vec2 v_texCoord;" << std::endl;
		shaderBuilder << "out vec4 fragColor;" << std::endl;
		shaderBuilder << "uniform sampler2D g_texture;" << std::endl;
		shaderBuilder << "void main()" << std::endl;
		shaderBuilder << "{" << std::endl;
		shaderBuilder << "	fragColor = texture(g_texture, v_texCoord);" << std::endl;
		shaderBuilder << "}" << std::endl;

		pixelShader.SetSource(shaderBuilder.str().c_str());
		bool result = pixelShader.Compile();
		assert(result);
	}

	auto program = std::make_shared<Framework::OpenGl::CProgram>();

	{
		program->AttachShader(vertexShader);
		program->AttachShader(pixelShader);
		bool result = program->Link();
		assert(result);
	}

	return program;
}
