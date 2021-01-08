#pragma once

#include <QtGui/QWindow>
#include "signal/Signal.h"

class OutputWindow : public QWindow
{
	Q_OBJECT
public:
	explicit OutputWindow(QWindow* parent = 0);
	~OutputWindow() = default;

	void ShowFullScreenCursor();
	void DismissFullScreenCursor();
	typedef Framework::CSignal<void(QWheelEvent*)> MouseWheelSignal;
	typedef Framework::CSignal<void(QMouseEvent*)> MouseMoveSignal;
	typedef Framework::CSignal<void(QMouseEvent*)> MousePressSignal;

	MouseWheelSignal onMouseWheelEvent;
	MouseMoveSignal onMouseMoveEvent;
	MousePressSignal onMousePressEvent;

protected:
	void exposeEvent(QExposeEvent* ev) Q_DECL_OVERRIDE;
	void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;
	void focusOutEvent(QFocusEvent*) Q_DECL_OVERRIDE;
	void focusInEvent(QFocusEvent*) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent*) Q_DECL_OVERRIDE;
	void mouseDoubleClickEvent(QMouseEvent*) Q_DECL_OVERRIDE;
	void wheelEvent(QWheelEvent*) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent*) Q_DECL_OVERRIDE;

signals:
	void keyDown(QKeyEvent*);
	void keyUp(QKeyEvent*);
	void focusOut(QFocusEvent*);
	void focusIn(QFocusEvent*);
	void doubleClick(QMouseEvent*);

private slots:
	void activeStateChanged();

private:
	bool m_fullScreenCursorActive = false;
	QTimer* m_fullScreenCursorTimer = nullptr;
};
