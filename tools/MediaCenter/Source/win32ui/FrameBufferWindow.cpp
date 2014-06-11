#include "FrameBufferWindow.h"

#define CLSNAME		_T("COutputWnd")

using namespace Framework;

PIXELFORMATDESCRIPTOR CFrameBufferWindow::m_PFD =
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

CFrameBufferWindow::CFrameBufferWindow(HWND hParent)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= NULL;
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		wc.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&wc);
	}

	RECT rect;
	::GetClientRect(hParent, &rect);
	Create(NULL, CLSNAME, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rect, hParent, NULL);
	SetClassPtr();

	m_frameBuffer = -1;
	m_texture = -1;
	m_texWidth = 0;
	m_texHeight = 0;

	InitializeRenderingContext();

	//UINT_PTR result = SetTimer(m_hWnd, 1, 16, NULL);
}

CFrameBufferWindow::~CFrameBufferWindow()
{
	if(m_frameBuffer != -1)
	{
		//glDeleteFramebuffersEXT(1, &m_frameBuffer);
	}
}

long CFrameBufferWindow::OnPaint()
{
//	PAINTSTRUCT ps;
//	BeginPaint(m_hWnd, &ps);
//	EndPaint(m_hWnd, &ps);
	return TRUE;
}

long CFrameBufferWindow::OnEraseBkgnd()
{
	return TRUE;
}

long CFrameBufferWindow::OnTimer(WPARAM timerId)
{
	wglMakeCurrent(m_hDC, m_hRC);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texture);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0f, 0.0f);
		
		glTexCoord2f(m_texWidth, 0.0f);
		glVertex2f(1.0f, 0.0f);

		glTexCoord2f(m_texWidth, m_texHeight);
		glVertex2f(1.0f, 1.0f);

		glTexCoord2f(0.0f, m_texHeight);
		glVertex2f(0.0f, 1.0f);
	}
	glEnd();
	glFlush();

	SwapBuffers(m_hDC);
	wglMakeCurrent(m_hDC, NULL);

	return FALSE;
}

void CFrameBufferWindow::SetViewportSize(unsigned int width, unsigned int height)
{
	wglMakeCurrent(m_hDC, m_hRC);

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	wglMakeCurrent(m_hDC, NULL);
}

void CFrameBufferWindow::SetImage(unsigned int width, unsigned int height, uint8* buffer)
{
	wglMakeCurrent(m_hDC, m_hRC);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
//	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

	m_texWidth = width;
	m_texHeight = height;

	wglMakeCurrent(m_hDC, NULL);

	OnTimer(0);
}

void CFrameBufferWindow::InitializeRenderingContext()
{
	m_hDC = GetDC(m_hWnd);
	unsigned int pf = ChoosePixelFormat(m_hDC, &m_PFD);
	SetPixelFormat(m_hDC, pf, &m_PFD);
	m_hRC = wglCreateContext(m_hDC);
	wglMakeCurrent(m_hDC, m_hRC);

#ifdef WIN32
	glewInit();
#endif

	//Initialize basic stuff
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	//glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	//Initialize fog
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0.0f);
	glFogf(GL_FOG_END, 1.0f);
	glHint(GL_FOG_HINT, GL_NICEST);
	glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);

	glDisable(GL_DEPTH_TEST);

	SetViewportSize(512, 384);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glGenTextures(1, &m_texture);

	//SetImage(400, 400, new uint8[400 * 400 * 4]);

#ifdef _WIREFRAME
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_TEXTURE_2D);
#endif

	wglMakeCurrent(m_hDC, NULL);

	//glGenFramebuffersEXT(1, &m_frameBuffer);
	//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_frameBuffer);
}
