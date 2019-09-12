#include "outputwindow.h"
#include <QResizeEvent>

OutputWindow::OutputWindow(QWindow* parent)
    : QWindow(parent)
{
	connect(this, SIGNAL(activeChanged()), this, SLOT(activeStateChanged()));
}

void OutputWindow::keyPressEvent(QKeyEvent* ev)
{
	emit keyDown(ev);
}

void OutputWindow::keyReleaseEvent(QKeyEvent* ev)
{
	emit keyUp(ev);
}

void OutputWindow::exposeEvent(QExposeEvent* ev)
{
	emit widthChanged(size().width());
	QWindow::exposeEvent(ev);
}

void OutputWindow::focusOutEvent(QFocusEvent* event)
{
	emit focusOut(event);
}
void OutputWindow::focusInEvent(QFocusEvent* event)
{
	emit focusIn(event);
}

void OutputWindow::activeStateChanged()
{
	if(isActive())
	{
		emit focusIn(new QFocusEvent(QEvent::FocusIn));
	}
	else
	{
		emit focusOut(new QFocusEvent(QEvent::FocusOut));
	}
}

void OutputWindow::mouseDoubleClickEvent(QMouseEvent* ev)
{
	emit doubleClick(ev);
}
