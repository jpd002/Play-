#if !defined(GLES_COMPATIBILITY)
#include "GSH_OpenGLQt.h"
#endif

#include <QSurface>
#include <QWindow>
#include <QOpenGLContext>

#if defined(GLES_COMPATIBILITY)
#include "GSH_OpenGLQt.h"
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

CGSH_OpenGLQt::CGSH_OpenGLQt(QSurface* renderSurface)
    : m_renderSurface(renderSurface)
{
}

CGSH_OpenGL::FactoryFunction CGSH_OpenGLQt::GetFactoryFunction(QSurface* renderSurface)
{
	return [renderSurface]() { return new CGSH_OpenGLQt(renderSurface); };
}

void CGSH_OpenGLQt::InitializeImpl()
{
#ifdef __APPLE__
	//When building against macOS SDK 10.15+ and running on macOS 10.15+, the system will trigger an assert because we
	//are not using NSOpenGLContext on the main thread. Disable this assert since it doesn't cause any problem if we do.
	CFPreferencesSetAppValue(CFSTR("NSOpenGLContextSuppressThreadAssertions"), kCFBooleanTrue, kCFPreferencesCurrentApplication);
#endif

	m_context = new QOpenGLContext();
	m_context->setFormat(m_renderSurface->format());

	bool succeeded = m_context->create();
	Q_ASSERT(succeeded);

	succeeded = m_context->makeCurrent(m_renderSurface);
	Q_ASSERT(succeeded);

#ifdef USE_GLEW
	glewExperimental = GL_TRUE;
	auto result = glewInit();
	Q_ASSERT(result == GLEW_OK);
#endif

	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLQt::ReleaseImpl()
{
	CGSH_OpenGL::ReleaseImpl();

	delete m_context;
}

void CGSH_OpenGLQt::PresentBackbuffer()
{
	bool swapBuffer = true;
	if(m_renderSurface->surfaceClass() == QSurface::Window)
	{
		swapBuffer = static_cast<QWindow*>(m_renderSurface)->isExposed();
	}

	if(swapBuffer)
	{
		m_context->swapBuffers(m_renderSurface);
		m_context->makeCurrent(m_renderSurface);
	}
}
