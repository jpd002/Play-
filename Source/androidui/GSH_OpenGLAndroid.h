#pragma once

#include "../GSH_OpenGL.h"
#include "opengl/OpenGlDef.h"

class CGSH_OpenGLAndroid : public CGSH_OpenGL
{
public:
							CGSH_OpenGLAndroid(NativeWindowType);
	virtual					~CGSH_OpenGLAndroid();
	
	static FactoryFunction 	GetFactoryFunction(NativeWindowType);
	
	void					PresentBackbuffer() override;
	
private:

};
