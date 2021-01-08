#pragma once

#include "gs/GSH_OpenGL/GSH_OpenGL.h"

class QSurface;
class QOpenGLContext;

class CGSH_OpenGLQt : public CGSH_OpenGL
{
public:
	CGSH_OpenGLQt(QSurface*);
	virtual ~CGSH_OpenGLQt() = default;

	static FactoryFunction GetFactoryFunction(QSurface*);

	void InitializeImpl() override;
	void ReleaseImpl() override;
	void PresentBackbuffer() override;

private:
	QSurface* m_renderSurface = nullptr;
	QOpenGLContext* m_context = nullptr;
};
