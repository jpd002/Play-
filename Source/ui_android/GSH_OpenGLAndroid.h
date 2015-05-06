#pragma once

#include "../gs/GSH_OpenGL/GSH_OpenGL.h"
#include "opengl/OpenGlDef.h"

class CGSH_OpenGLAndroid : public CGSH_OpenGL
{
public:
							CGSH_OpenGLAndroid(NativeWindowType);
	virtual					~CGSH_OpenGLAndroid();
	
	void					SetWindow(NativeWindowType);
	
	static FactoryFunction 	GetFactoryFunction(NativeWindowType);
	
	void					InitializeImpl() override;
	void					PresentBackbuffer() override;
	
private:
	void					SetupContext();
	
	NativeWindowType 		m_window = nullptr;
	EGLConfig				m_config = 0;
	EGLDisplay				m_display = EGL_NO_DISPLAY;
	EGLContext				m_context = EGL_NO_CONTEXT;
	EGLSurface				m_surface = EGL_NO_SURFACE;
};
