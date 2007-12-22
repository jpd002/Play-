#ifndef _GSH_OPENGLMACOSX_H_
#define _GSH_OPENGLMACOSX_H_

#include "../GSH_OpenGL.h"
#include "../PS2VM.h"

class CGSH_OpenGLMacOSX : public CGSH_OpenGL
{
public:
							CGSH_OpenGLMacOSX(CGLContextObj);
	virtual					~CGSH_OpenGLMacOSX();

	static void				CreateGSHandler(CPS2VM&, CGLContextObj);
	
	virtual void			InitializeImpl();
	virtual void			FlipImpl();
	
private:
	static CGSHandler*		GSHandlerFactory(void*);

	CGLContextObj			m_context;	
};

#endif
