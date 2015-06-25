#include <cassert>
#include "opengl/OpenGlDef.h"
#include "GSH_OpenGLiOS.h"
#include "../Log.h"

CGSH_OpenGLiOS::CGSH_OpenGLiOS(CAEAGLLayer* layer)
: m_layer(layer)
{

}

CGSH_OpenGLiOS::~CGSH_OpenGLiOS()
{
	
}

CGSHandler::FactoryFunction CGSH_OpenGLiOS::GetFactoryFunction(CAEAGLLayer* layer)
{
	return [layer]() { return new CGSH_OpenGLiOS(layer); };
}

void CGSH_OpenGLiOS::InitializeImpl()
{
	m_context = [[EAGLContext alloc] initWithAPI: kEAGLRenderingAPIOpenGLES3];
		
	if(!m_context)
	{
		NSLog(@"Failed to create ES context");
		return;
	}
	
	if(![EAGLContext setCurrentContext: m_context])
	{
		NSLog(@"Failed to set ES context current");
		return;
	}

	CreateFramebuffer();

	{
		PRESENTATION_PARAMS presentationParams;
		presentationParams.mode 			= PRESENTATION_MODE_FIT;
		presentationParams.windowWidth 		= m_framebufferWidth;
		presentationParams.windowHeight 	= m_framebufferHeight;

		SetPresentationParams(presentationParams);
	}

	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLiOS::PresentBackbuffer()
{
	glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
	CHECKGLERROR();
		
	BOOL success = [m_context presentRenderbuffer: GL_RENDERBUFFER];
	assert(success == YES);
}

void CGSH_OpenGLiOS::CreateFramebuffer()
{
	assert(m_defaultFramebuffer == 0);
	
	glGenFramebuffers(1, &m_defaultFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFramebuffer);
		
	glGenRenderbuffers(1, &m_depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
		
	// Create color render buffer and allocate backing store.
	glGenRenderbuffers(1, &m_colorRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
	[m_context renderbufferStorage: GL_RENDERBUFFER fromDrawable: m_layer];
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_framebufferWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_framebufferHeight);

	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_framebufferWidth, m_framebufferHeight);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRenderbuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);

	CHECKGLERROR();
	
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
		assert(false);
	}
	
	m_presentFramebuffer = m_defaultFramebuffer;
}
