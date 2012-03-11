#ifndef _GSH_OPENGLMACOSX_H_
#define _GSH_OPENGLMACOSX_H_

#include "../GSH_OpenGL.h"

class CGSH_OpenGLMacOSX : public CGSH_OpenGL
{
public:
							CGSH_OpenGLMacOSX(CGLContextObj);
	virtual					~CGSH_OpenGLMacOSX();

	static FactoryFunction	GetFactoryFunction(CGLContextObj);
	
	virtual void			InitializeImpl();
	virtual void			ReleaseImpl();
	virtual void			FlipImpl();
	virtual void			UpdateViewportImpl();
	
	virtual void			ReadFramebuffer(uint32, uint32, void*);

	virtual void			PresentBackbuffer();
	virtual void			LoadShaderSource(Framework::OpenGl::CShader*, SHADER);

protected:
	virtual void			ProcessImageTransfer(uint32, uint32);	
	
private:
	static CGSHandler*		GSHandlerFactory(CGLContextObj);

	CGLContextObj			m_context;	
};

#endif
