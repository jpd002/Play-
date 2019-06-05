#pragma once

#include "../GSH_OpenGL/GSH_OpenGL.h"
#include "win32/Window.h"

class CGSH_OpenGLWin32 : public CGSH_OpenGL
{
public:
	CGSH_OpenGLWin32(Framework::Win32::CWindow*);
	virtual ~CGSH_OpenGLWin32() = default;

	static FactoryFunction GetFactoryFunction(Framework::Win32::CWindow*);

	void InitializeImpl() override;
	void ReleaseImpl() override;

protected:
	void PresentBackbuffer() override;

private:
	static CGSHandler* GSHandlerFactory(Framework::Win32::CWindow*);

	Framework::Win32::CWindow* m_outputWnd = nullptr;

	HGLRC m_context = nullptr;
	HDC m_dc = nullptr;
	static PIXELFORMATDESCRIPTOR m_pfd;
};
