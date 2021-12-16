#include "GSH_OpenGLJs.h"

CGSH_OpenGLJs::CGSH_OpenGLJs(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context)
    : m_context(context)
{
}

CGSH_OpenGL::FactoryFunction CGSH_OpenGLJs::GetFactoryFunction(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context)
{
	return [context]() { return new CGSH_OpenGLJs(context); };
}

void CGSH_OpenGLJs::InitializeImpl()
{
	emscripten_webgl_make_context_current(m_context);
	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLJs::ReleaseImpl()
{
	CGSH_OpenGL::ReleaseImpl();
}

void CGSH_OpenGLJs::PresentBackbuffer()
{
}
