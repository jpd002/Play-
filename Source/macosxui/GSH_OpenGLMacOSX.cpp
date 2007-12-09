#include "GSH_OpenGLMacOSX.h"

CGSH_OpenGLMacOSX::CGSH_OpenGLMacOSX(CGLContextObj context) :
m_context(context)
{

}

CGSH_OpenGLMacOSX::~CGSH_OpenGLMacOSX()
{

}

void CGSH_OpenGLMacOSX::CreateGSHandler(CPS2VM& virtualMachine, CGLContextObj context)
{
	virtualMachine.CreateGSHandler(GSHandlerFactory, context);
}

void CGSH_OpenGLMacOSX::InitializeImpl()
{
	CGLSetCurrentContext(m_context);
	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLMacOSX::FlipImpl()
{
	glFlush();
	CGLFlushDrawable(m_context);
}

CGSHandler* CGSH_OpenGLMacOSX::GSHandlerFactory(void* param)
{
	return new CGSH_OpenGLMacOSX(reinterpret_cast<CGLContextObj>(param));
}
