#include <cassert>
#include "opengl/OpenGlDef.h"
#include "GSH_OpenGLAndroid.h"
#include "../Log.h"

CGSH_OpenGLAndroid::CGSH_OpenGLAndroid(NativeWindowType window)
{
	static const EGLint attribs[] = 
	{
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_BLUE_SIZE,		8,
		EGL_GREEN_SIZE,		8,
		EGL_RED_SIZE,		8,
		EGL_NONE
    };
	
	auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);
	
	EGLConfig config = 0;
	EGLint numConfigs = 0;
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	
	auto surface = eglCreateWindowSurface(display, config, window, NULL);
	assert(surface != EGL_NO_SURFACE);
	
#if 0
	auto context = eglCreateContext(display, config, NULL, NULL);
	assert(context != EGL_NO_CONTEXT);
	
	auto makeCurrentResult = eglMakeCurrent(display, surface, surface, context);
	assert(makeCurrentResult != EGL_FALSE);
	
	GLint w = 0, h = 0;
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);
	
	CLog::GetInstance().Print("gles", "w: %d, h: %d\r\n", w, h);
#endif
}

CGSH_OpenGLAndroid::~CGSH_OpenGLAndroid()
{
	
}

CGSHandler::FactoryFunction CGSH_OpenGLAndroid::GetFactoryFunction(NativeWindowType window)
{
	return [window]() { return new CGSH_OpenGLAndroid(window); };
}

void CGSH_OpenGLAndroid::PresentBackbuffer()
{

}
