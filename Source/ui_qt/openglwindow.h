#pragma once

#include "outputwindow.h"

class OpenGLWindow : public OutputWindow
{
	Q_OBJECT
public:
	explicit OpenGLWindow(QWindow* parent = 0);
	~OpenGLWindow() = default;
};
