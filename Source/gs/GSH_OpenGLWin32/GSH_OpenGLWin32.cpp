#include "GSH_OpenGLWin32.h"

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
        0, 0, 0};

CGSH_OpenGLWin32::CGSH_OpenGLWin32(Framework::Win32::CWindow* outputWindow)
    : m_outputWnd(outputWindow)
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

	auto createContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if(createContextAttribsARB != nullptr)
	{
		static const int attributes[] =
		    {
		        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		        WGL_CONTEXT_MINOR_VERSION_ARB, 2,
		        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		        0};

		auto newContext = createContextAttribsARB(m_dc, nullptr, attributes);
		assert(newContext != nullptr);

		if(newContext != nullptr)
		{
			auto prevContext = m_context;
			m_context = newContext;
			wglMakeCurrent(m_dc, m_context);

			auto deleteResult = wglDeleteContext(prevContext);
			assert(deleteResult == TRUE);
		}
	}

	//GLEW doesn't work well with core profiles, thus, we need to enable "experimental" to make
	//sure it properly gets all function pointers.
	glewExperimental = GL_TRUE;
	auto result = glewInit();
	assert(result == GLEW_OK);
	//Clear any error that might rise from GLEW getting function pointers
	glGetError();

	if(wglSwapIntervalEXT)
	{
		wglSwapIntervalEXT(-1);
	}

	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLWin32::ReleaseImpl()
{
	CGSH_OpenGL::ReleaseImpl();

	wglMakeCurrent(NULL, NULL);

	auto deleteResult = wglDeleteContext(m_context);
	assert(deleteResult == TRUE);
}

void CGSH_OpenGLWin32::PresentBackbuffer()
{
	SwapBuffers(m_dc);
}

CGSHandler* CGSH_OpenGLWin32::GSHandlerFactory(Framework::Win32::CWindow* outputWindow)
{
	return new CGSH_OpenGLWin32(outputWindow);
}
