#include "GSH_OpenGLWin32.h"
#include "RendererSettingsWnd.h"

PIXELFORMATDESCRIPTOR CGSH_OpenGLWin32::m_PFD =
{
	sizeof(PIXELFORMATDESCRIPTOR),
	1,
	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
	PFD_TYPE_RGBA,
	32,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0,
	32,
	0,
	0,
	PFD_MAIN_PLANE,
	0,
	0, 0, 0
};

CGSH_OpenGLWin32::CGSH_OpenGLWin32(Framework::Win32::CWindow* outputWindow)
: m_pOutputWnd(outputWindow)
{

}

CGSH_OpenGLWin32::~CGSH_OpenGLWin32()
{

}

CGSHandler::FactoryFunction CGSH_OpenGLWin32::GetFactoryFunction(Framework::Win32::CWindow* pOutputWnd)
{
	return std::bind(&CGSH_OpenGLWin32::GSHandlerFactory, pOutputWnd);
}

void CGSH_OpenGLWin32::InitializeImpl()
{
	m_hDC = GetDC(m_pOutputWnd->m_hWnd);
	unsigned int pf = ChoosePixelFormat(m_hDC, &m_PFD);
	SetPixelFormat(m_hDC, pf, &m_PFD);
	m_hRC = wglCreateContext(m_hDC);
	wglMakeCurrent(m_hDC, m_hRC);

	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLWin32::ReleaseImpl()
{
	CGSH_OpenGL::ReleaseImpl();

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
}

void CGSH_OpenGLWin32::PresentBackbuffer()
{
	SwapBuffers(m_hDC);
}

void CGSH_OpenGLWin32::SetViewport(int nWidth, int nHeight)
{
	RECT rc;

	SetRect(&rc, 0, 0, nWidth, nHeight);
	AdjustWindowRect(&rc, GetWindowLong(m_pOutputWnd->m_hWnd, GWL_STYLE), FALSE);
	m_pOutputWnd->SetSize((rc.right - rc.left), (rc.bottom - rc.top));

	CGSH_OpenGL::SetViewport(nWidth, nHeight);
}

CSettingsDialogProvider* CGSH_OpenGLWin32::GetSettingsDialogProvider()
{
	return this;
}

Framework::Win32::CModalWindow* CGSH_OpenGLWin32::CreateSettingsDialog(HWND hParent)
{
	return new CRendererSettingsWnd(hParent, this);
}

void CGSH_OpenGLWin32::OnSettingsDialogDestroyed()
{
	LoadSettings();
	TexCache_Flush();
	PalCache_Flush();
}

CGSHandler* CGSH_OpenGLWin32::GSHandlerFactory(Framework::Win32::CWindow* pParam)
{
	return new CGSH_OpenGLWin32(pParam);
}
