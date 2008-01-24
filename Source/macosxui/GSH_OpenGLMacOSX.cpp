#include "GSH_OpenGLMacOSX.h"

using namespace std::tr1;

CGSH_OpenGLMacOSX::CGSH_OpenGLMacOSX(CGLContextObj context) :
m_context(context)
{

}

CGSH_OpenGLMacOSX::~CGSH_OpenGLMacOSX()
{

}

CGSHandler::FactoryFunction CGSH_OpenGLMacOSX::GetFactoryFunction(CGLContextObj context)
{
	return bind(&CGSH_OpenGLMacOSX::GSHandlerFactory, context);
}

void CGSH_OpenGLMacOSX::InitializeImpl()
{
	CGLSetCurrentContext(m_context);
	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLMacOSX::FlipImpl()
{
	CGLFlushDrawable(m_context);
}

CGSHandler* CGSH_OpenGLMacOSX::GSHandlerFactory(CGLContextObj context)
{
	return new CGSH_OpenGLMacOSX(context);
}
