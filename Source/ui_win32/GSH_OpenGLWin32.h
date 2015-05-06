#pragma once

#include "../gs/GSH_OpenGL/GSH_OpenGL.h"
#include "win32/Window.h"
#include "SettingsDialogProvider.h"

class CGSH_OpenGLWin32 : public CGSH_OpenGL, public CSettingsDialogProvider
{
public:
									CGSH_OpenGLWin32(Framework::Win32::CWindow*);
	virtual							~CGSH_OpenGLWin32();

	static FactoryFunction			GetFactoryFunction(Framework::Win32::CWindow*);

	virtual void					InitializeImpl();
	virtual void					ReleaseImpl();

	CSettingsDialogProvider*		GetSettingsDialogProvider();

	Framework::Win32::CModalWindow* CreateSettingsDialog(HWND);
	void							OnSettingsDialogDestroyed();

protected:
	virtual void					PresentBackbuffer();

private:
	static CGSHandler*				GSHandlerFactory(Framework::Win32::CWindow*);

	Framework::Win32::CWindow*		m_outputWnd = nullptr;

	HGLRC							m_context = nullptr;
	HDC								m_dc = nullptr;
	static PIXELFORMATDESCRIPTOR	m_pfd;
};
