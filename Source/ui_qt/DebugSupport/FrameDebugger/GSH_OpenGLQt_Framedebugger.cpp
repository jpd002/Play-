#if !defined(GLES_COMPATIBILITY)
#include "GSH_OpenGLQt_Framedebugger.h"
#endif

#include <QWindow>
#include <QOpenGLContext>

#if defined(GLES_COMPATIBILITY)
#include "GSH_OpenGLQt_Framedebugger.h"
#endif

#include <assert.h>
#include <sstream>

#ifdef GLES_COMPATIBILITY
#define GLSL_VERSION "#version 300 es"
#else
#define GLSL_VERSION "#version 150"
#endif

CGSH_OpenGLFramedebugger::CGSH_OpenGLFramedebugger(QWindow* renderWindow)
    : m_renderWindow(renderWindow)
{
}

void CGSH_OpenGLFramedebugger::InitializeImpl()
{
	m_context = new QOpenGLContext();
	m_context->setFormat(m_renderWindow->requestedFormat());

	bool succeeded = m_context->create();
	Q_ASSERT(succeeded);

	succeeded = m_context->makeCurrent(m_renderWindow);
	Q_ASSERT(succeeded);

#ifdef USE_GLEW
	glewExperimental = GL_TRUE;
	auto result = glewInit();
	Q_ASSERT(result == GLEW_OK);
	//Clear any error that might rise from GLEW getting function pointers
#endif
	glGetError();

#ifdef __WIN32__
	if(wglSwapIntervalEXT)
	{
		wglSwapIntervalEXT(-1);
	}
#endif
}

void CGSH_OpenGLFramedebugger::ReleaseImpl()
{
	delete m_context;
}

void CGSH_OpenGLFramedebugger::Begin()
{
	m_context->makeCurrent(m_renderWindow);
}

void CGSH_OpenGLFramedebugger::PresentBackbuffer()
{
	if(m_renderWindow->isExposed())
	{
		m_context->swapBuffers(m_renderWindow);
		m_context->makeCurrent(m_renderWindow);
	}
}

void CGSH_OpenGLFramedebugger::PrepareFramedebugger()
{

	m_vertexArray = GenerateVertexArray();
	m_vertexBufferFramedebugger = GenerateVertexBuffer();

	m_checkerboardProgram = GenerateCheckerboardProgram();
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, reinterpret_cast<const GLvoid*>(0)); // POSITON

		m_checkerboardScreenSizeUniform = glGetUniformLocation(*m_checkerboardProgram, "g_screenSize");
	}

	m_pixelBufferViewProgram = GeneratePixelBufferViewProgram();
	{
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<const GLvoid*>(0));                 // POSITON
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<const GLvoid*>(sizeof(float) * 3)); // texCoord

		m_pixelBufferViewScreenSizeUniform = glGetUniformLocation(*m_pixelBufferViewProgram, "g_screenSize");
		m_pixelBufferViewBufferSizeUniform = glGetUniformLocation(*m_pixelBufferViewProgram, "g_bufferSize");
		m_pixelBufferViewPanOffsetUniform = glGetUniformLocation(*m_pixelBufferViewProgram, "g_panOffset");
		m_pixelBufferViewZoomFactorUniform = glGetUniformLocation(*m_pixelBufferViewProgram, "g_zoomFactor");
		m_pixelBufferViewtextureUniform = glGetUniformLocation(*m_pixelBufferViewProgram, "g_bufferTextureSampler");
	}
}

Framework::OpenGl::CVertexArray CGSH_OpenGLFramedebugger::GenerateVertexArray()
{
	auto vertexArray = Framework::OpenGl::CVertexArray::Create();
	glBindVertexArray(vertexArray);

	CHECKGLERROR();

	return vertexArray;
}

Framework::OpenGl::CBuffer CGSH_OpenGLFramedebugger::GenerateVertexBuffer()
{
	auto buffer = Framework::OpenGl::CBuffer::Create();

	// clang-format off
	static float vertices[] =
	{
		 1.0f,  1.0f, 0.0f,		1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f,		0.0f, 1.0f,

		-1.0f, -1.0f, 0.0f,		0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,		0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,		1.0f, 0.0f,

	};
	// clang-format on

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	CHECKGLERROR();

	return buffer;
}

Framework::OpenGl::ProgramPtr CGSH_OpenGLFramedebugger::GenerateCheckerboardProgram()
{
	auto vertexShader = GenerateCheckerboardVertexShader();
	auto fragmentShader = GenerateCheckerboardFragmentShader();

	auto result = std::make_shared<Framework::OpenGl::CProgram>();

	result->AttachShader(vertexShader);
	result->AttachShader(fragmentShader);

	FRAMEWORK_MAYBE_UNUSED bool linkResult = result->Link();
	assert(linkResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGLFramedebugger::GenerateCheckerboardVertexShader()
{

	std::stringstream shaderBuilder;
	shaderBuilder << GLSL_VERSION << std::endl;
	shaderBuilder << "#extension GL_ARB_explicit_attrib_location : enable" << std::endl
	              << std::endl;

	shaderBuilder << "layout (location = 0) in vec3 g_position;" << std::endl;
	shaderBuilder << "out vec2 a_texCoord;" << std::endl;
	shaderBuilder << "uniform vec2 g_screenSize;" << std::endl;

	shaderBuilder << "void main()" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	gl_Position = vec4(g_position, 1.0);" << std::endl;
	shaderBuilder << "	a_texCoord = ((g_position.xy + 1.0f) / 2.0f) * g_screenSize / 10.f;" << std::endl;
	shaderBuilder << "}" << std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_VERTEX_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	FRAMEWORK_MAYBE_UNUSED bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGLFramedebugger::GenerateCheckerboardFragmentShader()
{
	std::stringstream shaderBuilder;

	shaderBuilder << GLSL_VERSION << std::endl
	              << std::endl;

	shaderBuilder << "precision mediump float;" << std::endl;

	shaderBuilder << "out vec4 fragColor;" << std::endl;
	shaderBuilder << "in vec2 a_texCoord;" << std::endl;

	shaderBuilder << "float fmod(float x, float y) { return x - y * trunc(x/y); }" << std::endl;
	shaderBuilder << "void main()" << std::endl;
	shaderBuilder << "{" << std::endl;

	shaderBuilder << "	if(fmod(floor(a_texCoord.x) + floor(a_texCoord.y), 2.0) < 1.0)" << std::endl;
	shaderBuilder << "	{" << std::endl;
	shaderBuilder << "		fragColor =  vec4(1, 1, 1, 1);" << std::endl;
	shaderBuilder << "	}" << std::endl;
	shaderBuilder << "	else" << std::endl;
	shaderBuilder << "	{" << std::endl;
	shaderBuilder << "		fragColor =  vec4(0.75, 0.75, 0.75, 1.0);" << std::endl;
	shaderBuilder << "	}" << std::endl;

	shaderBuilder << "}" << std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_FRAGMENT_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	FRAMEWORK_MAYBE_UNUSED bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

	return result;
}

void CGSH_OpenGLFramedebugger::DrawCheckerboard(float* dim)
{
	glViewport(0, 0, dim[0], dim[1]);

	glUseProgram(*m_checkerboardProgram);

	glBindVertexArray(m_vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferFramedebugger);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUniform2f(m_checkerboardScreenSizeUniform, dim[0], dim[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

Framework::OpenGl::ProgramPtr CGSH_OpenGLFramedebugger::GeneratePixelBufferViewProgram()
{
	auto vertexShader = GeneratePixelBufferViewVertexShader();
	auto fragmentShader = GeneratePixelBufferViewFragmentShader();

	auto result = std::make_shared<Framework::OpenGl::CProgram>();

	result->AttachShader(vertexShader);
	result->AttachShader(fragmentShader);

	FRAMEWORK_MAYBE_UNUSED bool linkResult = result->Link();
	assert(linkResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGLFramedebugger::GeneratePixelBufferViewVertexShader()
{

	std::stringstream shaderBuilder;
	shaderBuilder << GLSL_VERSION << std::endl
	              << std::endl;
	shaderBuilder << "#extension GL_ARB_explicit_attrib_location : enable" << std::endl
	              << std::endl;

	shaderBuilder << "layout (location = 0) in vec3 g_position;" << std::endl;
	shaderBuilder << "layout (location = 1) in vec2 g_texCoord;" << std::endl;
	shaderBuilder << "uniform vec2 g_screenSize;" << std::endl;
	shaderBuilder << "uniform vec2 g_bufferSize;" << std::endl;
	shaderBuilder << "uniform vec2 g_panOffset;" << std::endl;
	shaderBuilder << "uniform float g_zoomFactor;" << std::endl;
	shaderBuilder << "out vec2 a_texCoord;" << std::endl;
	shaderBuilder << "" << std::endl;
	shaderBuilder << "void main()" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	gl_Position = ((vec4(g_position, 1.0) * vec4(g_bufferSize / g_screenSize, 0, 1)) + vec4(g_panOffset, 0, 0)) * vec4(g_zoomFactor, g_zoomFactor, 0, 1);" << std::endl;
	shaderBuilder << "	a_texCoord = g_texCoord;" << std::endl;
	shaderBuilder << "}" << std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_VERTEX_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	FRAMEWORK_MAYBE_UNUSED bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

	return result;
}

Framework::OpenGl::CShader CGSH_OpenGLFramedebugger::GeneratePixelBufferViewFragmentShader()
{
	std::stringstream shaderBuilder;

	shaderBuilder << GLSL_VERSION << std::endl
	              << std::endl;

	shaderBuilder << "precision mediump float;" << std::endl;

	shaderBuilder << "uniform sampler2D g_bufferTextureSampler;" << std::endl;
	shaderBuilder << "in vec2 a_texCoord;" << std::endl;
	shaderBuilder << "out vec4 fragColor;" << std::endl;

	shaderBuilder << "void main()" << std::endl;
	shaderBuilder << "{" << std::endl;
	shaderBuilder << "	fragColor = texture(g_bufferTextureSampler, a_texCoord);" << std::endl;
	shaderBuilder << "}" << std::endl;

	auto shaderSource = shaderBuilder.str();

	Framework::OpenGl::CShader result(GL_FRAGMENT_SHADER);
	result.SetSource(shaderSource.c_str(), shaderSource.size());
	FRAMEWORK_MAYBE_UNUSED bool compilationResult = result.Compile();
	assert(compilationResult);

	CHECKGLERROR();

	return result;
}

void CGSH_OpenGLFramedebugger::DrawPixelBuffer(float* screenSizeVector, float* bufferSizeVector, float panX, float panY, float zoomFactor)
{
	if(m_activeTexture == -1)
		return;

	float panOffsetVector[2] = {panX, panY};

	glUseProgram(*m_pixelBufferViewProgram);
	glBindVertexArray(m_vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferFramedebugger);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glUniform2f(m_pixelBufferViewScreenSizeUniform, screenSizeVector[0], screenSizeVector[1]);

	glUniform2f(m_pixelBufferViewBufferSizeUniform, bufferSizeVector[0], bufferSizeVector[1]);
	glUniform2f(m_pixelBufferViewPanOffsetUniform, panOffsetVector[0], panOffsetVector[1]);
	glUniform1f(m_pixelBufferViewZoomFactorUniform, zoomFactor);

	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, m_activeTexture);
	glUniform1i(m_pixelBufferViewtextureUniform, 4);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void CGSH_OpenGLFramedebugger::LoadTextureFromBitmap(const Framework::CBitmap& bitmap)
{
	if(!bitmap.IsEmpty())
	{
		std::vector<GLint> textureFormat =
		    [bitmap]() -> std::vector<GLint> {
			switch(bitmap.GetBitsPerPixel())
			{
			case 8:
				return {GL_R8, GL_RED};
			case 16:
				return {GL_RGB5_A1, GL_BGRA};
			case 32:
			default:
				return {GL_RGBA8, GL_BGRA};
			}
		}();

		auto type = GL_UNSIGNED_BYTE;
		if(bitmap.GetBitsPerPixel() == 16)
		{
			type = GL_UNSIGNED_SHORT_5_5_5_1;
		}
		glActiveTexture(GL_TEXTURE0 + 4);
		glGenTextures(1, &m_activeTexture);
		glBindTexture(GL_TEXTURE_2D, m_activeTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, textureFormat[0], bitmap.GetWidth(), bitmap.GetHeight(), 0, textureFormat[1], type, bitmap.GetPixels());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		if(bitmap.GetBitsPerPixel() == 8)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		CHECKGLERROR();
	}
}