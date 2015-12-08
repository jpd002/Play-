#include "GSH_OpenGLWin32.h"
#include "GSH_OpenGL_SettingsWnd.h"

PIXELFORMATDESCRIPTOR CGSH_OpenGLWin32::m_pfd =
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
: m_outputWnd(outputWindow)
{

}

CGSH_OpenGLWin32::~CGSH_OpenGLWin32()
{

}

CGSHandler::FactoryFunction CGSH_OpenGLWin32::GetFactoryFunction(Framework::Win32::CWindow* outputWindow)
{
	return std::bind(&CGSH_OpenGLWin32::GSHandlerFactory, outputWindow);
}

void CGSH_OpenGLWin32::InitializeImpl()
{
	m_dc = GetDC(m_outputWnd->m_hWnd);
	unsigned int pf = ChoosePixelFormat(m_dc, &m_pfd);
	SetPixelFormat(m_dc, pf, &m_pfd);
	m_context = wglCreateContext(m_dc);
	wglMakeCurrent(m_dc, m_context);

	auto result = glewInit();
	assert(result == GLEW_OK);

	if(wglCreateContextAttribsARB != nullptr)
	{
		auto prevContext = m_context;

		static const int attributes[] = 
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 2,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		m_context = wglCreateContextAttribsARB(m_dc, nullptr, attributes);
		assert(m_context != nullptr);

		wglMakeCurrent(m_dc, m_context);
		auto deleteResult = wglDeleteContext(prevContext);
		assert(deleteResult == TRUE);
	}

	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLWin32::ReleaseImpl()
{
	CGSH_OpenGL::ReleaseImpl();

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_context);
}

void CGSH_OpenGLWin32::PresentBackbuffer()
{
	SwapBuffers(m_dc);
}

CSettingsDialogProvider* CGSH_OpenGLWin32::GetSettingsDialogProvider()
{
	return this;
}

Framework::Win32::CWindow* CGSH_OpenGLWin32::CreateSettingsDialog(HWND parentWnd)
{
	return new CGSH_OpenGL_SettingsWnd(parentWnd);
}

void CGSH_OpenGLWin32::OnSettingsDialogDestroyed()
{
	LoadSettings();
	TexCache_Flush();
	PalCache_Flush();
}

CGSHandler* CGSH_OpenGLWin32::GSHandlerFactory(Framework::Win32::CWindow* outputWindow)
{
	return new CGSH_OpenGLWin32(outputWindow);
}
