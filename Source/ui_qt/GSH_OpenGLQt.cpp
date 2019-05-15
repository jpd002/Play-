#include "GSH_OpenGLQt.h"
#include <QWindow>
#include <QOpenGLContext>

CGSH_OpenGLQt::CGSH_OpenGLQt(QWindow* renderWindow)
    : m_renderWindow(renderWindow)
{
}

CGSH_OpenGLQt::~CGSH_OpenGLQt()
{
}

CGSH_OpenGL::FactoryFunction CGSH_OpenGLQt::GetFactoryFunction(QWindow* renderWindow)
{
	return [renderWindow]() { return new CGSH_OpenGLQt(renderWindow); };
}

void CGSH_OpenGLQt::InitializeImpl()
{
	m_context = new QOpenGLContext();
	m_context->setFormat(m_renderWindow->requestedFormat());

	bool succeeded = m_context->create();
	Q_ASSERT(succeeded);

	succeeded = m_context->makeCurrent(m_renderWindow);
	Q_ASSERT(succeeded);

#if !defined(__APPLE__) && !(defined(__aarch64__) && defined(__linux__))
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
	if(m_renderWindow->isExposed())
	{
		m_context->swapBuffers(m_renderWindow);
		m_context->makeCurrent(m_renderWindow);
	}
}
