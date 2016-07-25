#ifndef GSH_OPENGLUNIX_H
#define GSH_OPENGLUNIX_H


#pragma once

#include "gs/GSH_OpenGL/GSH_OpenGL.h"
#include "opengl/OpenGlDef.h"
#include <GL/glx.h>

class CGSH_OpenGLUnix : public CGSH_OpenGL
{
public:
                            CGSH_OpenGLUnix(Window);
    virtual					~CGSH_OpenGLUnix();


    static FactoryFunction 	GetFactoryFunction(Window);

    void					InitializeImpl() override;
    void					PresentBackbuffer() override;


private:
    void					SetupContext();
    void					CreateFramebuffer();

    GLuint					m_defaultFramebuffer = 0;
    GLuint					m_colorRenderbuffer = 0;
    GLint					m_framebufferWidth = 720;
    GLint					m_framebufferHeight = 480;

    Window			 	m_NativeWindow = 0;

    Display*			m_NativeDisplay;
    GLXContext			m_context;


    static CGSHandler* GSHandlerFactory(Window);


};

#endif // GSH_OPENGLUNIX_H
