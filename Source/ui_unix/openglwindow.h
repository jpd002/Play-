#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QtGui/QWindow>

class OpenGLWindow : public QWindow
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0);
        ~OpenGLWindow();

protected:
    void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;

signals:
    void keyDown(QKeyEvent*);
    void keyUp(QKeyEvent*);

};

#endif // OPENGLWINDOW_H
