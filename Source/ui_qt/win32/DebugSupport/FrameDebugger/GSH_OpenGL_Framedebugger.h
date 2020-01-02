#pragma once

#include "gs/GSH_OpenGL/GSH_OpenGL.h"
#include "win32/Window.h"

class CGSH_OpenGLFramedebugger
{
public:
	CGSH_OpenGLFramedebugger(Framework::Win32::CWindow*);
	virtual ~CGSH_OpenGLFramedebugger() = default;

	void InitializeImpl();
	void ReleaseImpl();
	void PresentBackbuffer();
	void Begin();

	void DrawCheckerboard(float*);
	void DrawPixelBuffer(float*, float*, float, float, float);
	void LoadTextureFromBitmap(const Framework::CBitmap&);


private:
	Framework::Win32::CWindow* m_outputWnd = nullptr;

	HGLRC m_context = nullptr;
	HDC m_dc = nullptr;
	static PIXELFORMATDESCRIPTOR m_pfd;

	Framework::OpenGl::CBuffer m_vertexBufferFramedebugger;
	Framework::OpenGl::ProgramPtr m_checkerboardProgram;
	Framework::OpenGl::ProgramPtr m_pixelBufferViewProgram;
	Framework::OpenGl::CVertexArray m_vertexArray;

	GLint m_checkerboardScreenSizeUniform = -1 ;
	GLint m_pixelBufferViewScreenSizeUniform = -1 ;
	GLint m_pixelBufferViewBufferSizeUniform = -1 ;
	GLint m_pixelBufferViewPanOffsetUniform = -1 ;
	GLint m_pixelBufferViewZoomFactorUniform = -1 ;
	GLint m_pixelBufferViewtextureUniform = -1 ;

	Framework::OpenGl::CVertexArray GenerateVertexArray();

	void PrepareFramedebugger();
	Framework::OpenGl::CBuffer GenerateVertexBuffer();

	Framework::OpenGl::ProgramPtr GenerateCheckerboardProgram();
	Framework::OpenGl::CShader GenerateCheckerboardVertexShader();
	Framework::OpenGl::CShader GenerateCheckerboardFragmentShader();

	Framework::OpenGl::ProgramPtr GeneratePixelBufferViewProgram();
	Framework::OpenGl::CShader GeneratePixelBufferViewVertexShader();
	Framework::OpenGl::CShader GeneratePixelBufferViewFragmentShader();


	GLuint m_activeTexture;

};
