#include "openglwindow.h"
#include <QResizeEvent>

OpenGLWindow::OpenGLWindow(QWindow *parent)
    : QWindow(parent)
{
    QSurfaceFormat format;
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);

    setSurfaceType(QWindow::OpenGLSurface);
    setFormat(format);
}

OpenGLWindow::~OpenGLWindow()
{
}

void OpenGLWindow::keyPressEvent(QKeyEvent *ev)
{
    emit keyDown(ev);
}

void OpenGLWindow::keyReleaseEvent(QKeyEvent *ev)
{
    emit keyUp(ev);
}
