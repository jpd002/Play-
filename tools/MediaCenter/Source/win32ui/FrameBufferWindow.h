#ifndef _FRAMEBUFFERWINDOW_H_
#define _FRAMEBUFFERWINDOW_H_

#include "Types.h"
#include "win32/Window.h"
#include "opengl/OpenGlDef.h"

class CFrameBufferWindow : public Framework::Win32::CWindow
{
public:
									CFrameBufferWindow(HWND);
	virtual							~CFrameBufferWindow();

	void							SetViewportSize(unsigned int, unsigned int);
	void							SetImage(unsigned int, unsigned int, uint8*);

	long							OnPaint();
	long							OnEraseBkgnd();
	long							OnTimer(WPARAM);

private:
	void							InitializeRenderingContext();

	HDC								m_hDC;
	HGLRC							m_hRC;
	static PIXELFORMATDESCRIPTOR	m_PFD;
	GLuint							m_frameBuffer;
	GLuint							m_texture;
	GLfloat							m_texWidth;
	GLfloat							m_texHeight;
};

#endif
