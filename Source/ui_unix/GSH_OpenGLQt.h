#pragma once

#include "gs/GSH_OpenGL/GSH_OpenGL.h"

class QWindow;
class QOpenGLContext;

class CGSH_OpenGLQt : public CGSH_OpenGL
{
public:
	CGSH_OpenGLQt(QWindow*);
	virtual ~CGSH_OpenGLQt();

	static FactoryFunction GetFactoryFunction(QWindow*);

	void InitializeImpl() override;
	void ReleaseImpl() override;
	void PresentBackbuffer() override;

private:
	QWindow* m_renderWindow = nullptr;
	QOpenGLContext* m_context = nullptr;
};
