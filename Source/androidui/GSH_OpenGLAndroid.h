#pragma once

#include "../GSH_OpenGL.h"
#include "opengl/OpenGlDef.h"

class CGSH_OpenGLAndroid : public CGSH_OpenGL
{
public:
							CGSH_OpenGLAndroid(NativeWindowType);
	virtual					~CGSH_OpenGLAndroid();
	
	static FactoryFunction 	GetFactoryFunction(NativeWindowType);
	
	void					InitializeImpl() override;
	void					PresentBackbuffer() override;
	
private:
	NativeWindowType 		m_window = nullptr;
	EGLDisplay				m_display = EGL_NO_DISPLAY;
	EGLSurface				m_surface = EGL_NO_SURFACE;
};
