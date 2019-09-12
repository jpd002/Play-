#include "openglwindow.h"

OpenGLWindow::OpenGLWindow(QWindow* parent)
    : OutputWindow(parent)
{
	QSurfaceFormat format;
#if defined(GLES_COMPATIBILITY)
	format.setVersion(3, 0);
#else
	format.setVersion(3, 2);
#endif
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

	setSurfaceType(QWindow::OpenGLSurface);
	setFormat(format);
}
