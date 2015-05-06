#include "GSH_OpenGLMacOSX.h"
#include "CoreFoundation/CoreFoundation.h"
#include "StdStream.h"

CGSH_OpenGLMacOSX::CGSH_OpenGLMacOSX(CGLContextObj context)
: m_context(context)
{

}

CGSH_OpenGLMacOSX::~CGSH_OpenGLMacOSX()
{

}

CGSHandler::FactoryFunction CGSH_OpenGLMacOSX::GetFactoryFunction(CGLContextObj context)
{
	return std::bind(&CGSH_OpenGLMacOSX::GSHandlerFactory, context);
}

void CGSH_OpenGLMacOSX::InitializeImpl()
{
	CGLSetCurrentContext(m_context);
	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLMacOSX::ReleaseImpl()
{
	
}

void CGSH_OpenGLMacOSX::ReadFramebuffer(uint32, uint32, void*)
{
	
}

void CGSH_OpenGLMacOSX::PresentBackbuffer()
{
	CGLFlushDrawable(m_context);
}

CGSHandler* CGSH_OpenGLMacOSX::GSHandlerFactory(CGLContextObj context)
{
	return new CGSH_OpenGLMacOSX(context);
}
