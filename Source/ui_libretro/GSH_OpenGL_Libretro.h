#pragma once

#include "gs/GSH_OpenGL/GSH_OpenGL.h"
#include "libretro.h"

class CGSH_OpenGL_Libretro : public CGSH_OpenGL
{
public:
	CGSH_OpenGL_Libretro();
	virtual ~CGSH_OpenGL_Libretro();

	static FactoryFunction GetFactoryFunction();

	void InitializeImpl() override;
	void FlipImpl() override;
	void Reset();
	void Release();
	void PresentBackbuffer() override;
	void UpdatePresentation();
	void FlushMailBox();

private:
	void UpdatePresentationImpl();

	GLint m_framebufferWidth = 512;
	GLint m_framebufferHeight = 512;
};
