#pragma once

#include "../gs/GSH_OpenGL/GSH_OpenGL.h"
#include "opengl/OpenGlDef.h"

class CGSH_OpenGLiOS : public CGSH_OpenGL
{
public:
							CGSH_OpenGLiOS(CAEAGLLayer*);
	virtual					~CGSH_OpenGLiOS();
	
	static FactoryFunction 	GetFactoryFunction(CAEAGLLayer*);
	
	void					InitializeImpl() override;
	void					PresentBackbuffer() override;

private:
	void					CreateFramebuffer();
	
	CAEAGLLayer*			m_layer = nullptr;
	EAGLContext*			m_context = nullptr;
	GLuint					m_defaultFramebuffer = 0;
	GLuint					m_colorRenderbuffer = 0;
	GLuint					m_depthRenderbuffer = 0;
	
	GLint					m_framebufferWidth = 0;
	GLint					m_framebufferHeight = 0;
};
