#pragma once

#include "bitmap/Bitmap.h"
#include "opengl/OpenGlDef.h"
#include "opengl/Program.h"
#include "opengl/Shader.h"
#include "opengl/Resource.h"

class QWindow;
class QOpenGLContext;

class CGSH_OpenGLFramedebugger
{
public:
	CGSH_OpenGLFramedebugger(QWindow*);
	virtual ~CGSH_OpenGLFramedebugger() = default;

	void InitializeImpl();
	void PrepareFramedebugger();

	void ReleaseImpl();
	void PresentBackbuffer();
	void Begin();

	void DrawCheckerboard(float*);
	void DrawPixelBuffer(float*, float*, float, float, float);
	void LoadTextureFromBitmap(const Framework::CBitmap&);

private:
	QWindow* m_renderWindow = nullptr;
	QOpenGLContext* m_context = nullptr;

	Framework::OpenGl::CBuffer m_vertexBufferFramedebugger;
	Framework::OpenGl::ProgramPtr m_checkerboardProgram;
	Framework::OpenGl::ProgramPtr m_pixelBufferViewProgram;
	Framework::OpenGl::CVertexArray m_vertexArray;

	GLint m_checkerboardScreenSizeUniform = -1;
	GLint m_pixelBufferViewScreenSizeUniform = -1;
	GLint m_pixelBufferViewBufferSizeUniform = -1;
	GLint m_pixelBufferViewPanOffsetUniform = -1;
	GLint m_pixelBufferViewZoomFactorUniform = -1;
	GLint m_pixelBufferViewtextureUniform = -1;

	Framework::OpenGl::CVertexArray GenerateVertexArray();

	Framework::OpenGl::CBuffer GenerateVertexBuffer();

	Framework::OpenGl::ProgramPtr GenerateCheckerboardProgram();
	Framework::OpenGl::CShader GenerateCheckerboardVertexShader();
	Framework::OpenGl::CShader GenerateCheckerboardFragmentShader();

	Framework::OpenGl::ProgramPtr GeneratePixelBufferViewProgram();
	Framework::OpenGl::CShader GeneratePixelBufferViewVertexShader();
	Framework::OpenGl::CShader GeneratePixelBufferViewFragmentShader();

	GLuint m_activeTexture;
};
