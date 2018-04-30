#ifndef _GSH_OPENGLMACOSX_H_
#define _GSH_OPENGLMACOSX_H_

#include "../gs/GSH_OpenGL/GSH_OpenGL.h"

class CGSH_OpenGLMacOSX : public CGSH_OpenGL
{
public:
	CGSH_OpenGLMacOSX(CGLContextObj);
	virtual ~CGSH_OpenGLMacOSX();

	static FactoryFunction GetFactoryFunction(CGLContextObj);

	virtual void InitializeImpl() override;
	virtual void ReleaseImpl() override;

	virtual void ReadFramebuffer(uint32, uint32, void*) override;

protected:
	virtual void PresentBackbuffer() override;

private:
	static CGSHandler* GSHandlerFactory(CGLContextObj);

	CGLContextObj m_context;
};

#endif
