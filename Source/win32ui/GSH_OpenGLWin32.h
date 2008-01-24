#ifndef _GSH_OPENGLWIN32_H_
#define _GSH_OPENGLWIN32_H_

#include "../GSH_OpenGL.h"
#include "win32/Window.h"
#include "SettingsDialogProvider.h"

class CGSH_OpenGLWin32 : public CGSH_OpenGL, public CSettingsDialogProvider
{
public:
    							    CGSH_OpenGLWin32(Framework::Win32::CWindow*);
    virtual                         ~CGSH_OpenGLWin32();

    static FactoryFunction          GetFactoryFunction(Framework::Win32::CWindow*);

    virtual void                    InitializeImpl();
	virtual void					FlipImpl();

    CSettingsDialogProvider*		GetSettingsDialogProvider();

	CModalWindow*					CreateSettingsDialog(HWND);
	void							OnSettingsDialogDestroyed();

private:
    virtual void                    SetViewport(int, int);

    static CGSHandler*				GSHandlerFactory(Framework::Win32::CWindow*);

    Framework::Win32::CWindow*		m_pOutputWnd;

	HGLRC							m_hRC;
	HDC								m_hDC;
	static PIXELFORMATDESCRIPTOR	m_PFD;
};

#endif
