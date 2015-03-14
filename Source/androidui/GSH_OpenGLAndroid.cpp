#include <cassert>
#include "opengl/OpenGlDef.h"
#include "GSH_OpenGLAndroid.h"
#include "../Log.h"

CGSH_OpenGLAndroid::CGSH_OpenGLAndroid(NativeWindowType window)
: m_window(window)
{
	//We'll probably need to keep 'window' as a Global JNI object
}

CGSH_OpenGLAndroid::~CGSH_OpenGLAndroid()
{
	
}

CGSHandler::FactoryFunction CGSH_OpenGLAndroid::GetFactoryFunction(NativeWindowType window)
{
	return [window]() { return new CGSH_OpenGLAndroid(window); };
}

void CGSH_OpenGLAndroid::InitializeImpl()
{
	static const EGLint attribs[] = 
	{
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_BLUE_SIZE,			8,
		EGL_GREEN_SIZE,			8,
		EGL_RED_SIZE,			8,
		EGL_NONE
	};
	
	m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(m_display, 0, 0);
	
	EGLConfig config = 0;
	EGLint numConfigs = 0;
	eglChooseConfig(m_display, attribs, &config, 1, &numConfigs);
	
	m_surface = eglCreateWindowSurface(m_display, config, m_window, NULL);
	assert(m_surface != EGL_NO_SURFACE);
	
	auto context = eglCreateContext(m_display, config, NULL, NULL);
	assert(context != EGL_NO_CONTEXT);
	
	auto makeCurrentResult = eglMakeCurrent(m_display, m_surface, m_surface, context);
	assert(makeCurrentResult != EGL_FALSE);
	
	{
		GLint w = 0, h = 0;
		eglQuerySurface(m_display, m_surface, EGL_WIDTH, &w);
		eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &h);
		
		PRESENTATION_PARAMS presentationParams;
		presentationParams.mode 			= PRESENTATION_MODE_FIT;
		presentationParams.windowWidth 		= w;
		presentationParams.windowHeight 	= h;

		SetPresentationParams(presentationParams);
	}
	
	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLAndroid::PresentBackbuffer()
{
	eglSwapBuffers(m_display, m_surface);
}
