#include "openglwindow.h"

OpenGLWindow::OpenGLWindow(QWindow* parent)
    : OutputWindow(parent)
{
	setSurfaceType(QWindow::OpenGLSurface);
	setFormat(GetSurfaceFormat());
}

QSurfaceFormat OpenGLWindow::GetSurfaceFormat()
{
	QSurfaceFormat format;
#if defined(GLES_COMPATIBILITY)
	format.setVersion(3, 0);
#else
	format.setVersion(3, 2);
#endif
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	format.setSwapInterval(0);
	format.setAlphaBufferSize(0);
	return format;
}
