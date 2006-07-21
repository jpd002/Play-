#include <boost/bind.hpp>
#include "SaveView.h"
#include "win32/ClientDeviceContext.h"
#include "opengl/OpenGl.h"

#define CLSNAME		_X("CSaveView_CIconView")

using namespace Framework;
using namespace boost;

PIXELFORMATDESCRIPTOR	CSaveView::CIconView::m_PFD =
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

CSaveView::CIconView::CIconView(HWND hParent, RECT* pR)
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

	Create(WS_EX_STATICEDGE, CLSNAME, _X(""), WS_VISIBLE | WS_CLIPCHILDREN | WS_CHILD, pR, hParent, NULL);
	SetClassPtr();

	m_pThread = new thread(bind(&CSaveView::CIconView::ThreadProc, this));
}

CSaveView::CIconView::~CIconView()
{
	m_MailSlot.SendMessage(THREAD_END, NULL);
	m_pThread->join();
	delete m_pThread;
}

void CSaveView::CIconView::ThreadProc()
{
	Win32::CClientDeviceContext DeviceContext(m_hWnd);
	unsigned int nPixelFormat;
	bool nEnd;

	nPixelFormat = ChoosePixelFormat(DeviceContext, &m_PFD);
	SetPixelFormat(DeviceContext, nPixelFormat, &m_PFD);
	m_hRC = wglCreateContext(DeviceContext);
	wglMakeCurrent(DeviceContext, m_hRC);

	glEnable(GL_TEXTURE_2D);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	
	nEnd = false;
	while(!nEnd)
	{
		if(m_MailSlot.IsMessagePending())
		{
			CThreadMsg::MESSAGE Message;
			m_MailSlot.GetMessage(&Message);
			switch(Message.nMsg)
			{
			case THREAD_END:
				nEnd = true;
				break;
			}
			m_MailSlot.FlushMessage(0);
		}
		else
		{
			Render(DeviceContext);
			Sleep(16);
		}
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
}

void CSaveView::CIconView::Render(HDC hDC)
{
	RECT ClientRect;
	GetClientRect(&ClientRect);

	glViewport(0, 0, ClientRect.right, ClientRect.bottom);

	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBegin(GL_QUADS);
	{
		glColor4d(0.5, 0.5, 0.5, 1.0);
		glVertex2d(0.0, 0.0);
		glVertex2d(1.0, 0.0);

		glColor4d(1.0, 1.0, 1.0, 1.0);
		glVertex2d(1.0, 0.5);
		glVertex2d(0.0, 0.5);

		glVertex2d(1.0, 0.5);
		glVertex2d(0.0, 0.5);

		glColor4d(0.5, 0.5, 0.5, 1.0);
		glVertex2d(0.0, 1.0);
		glVertex2d(1.0, 1.0);
	}
	glEnd();

	SwapBuffers(hDC);
}
