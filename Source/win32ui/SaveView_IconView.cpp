#include <boost/bind.hpp>
#include "resource.h"
#include "SaveView.h"
#include "win32/ClientDeviceContext.h"
#include "opengl/OpenGl.h"
#include "PtrMacro.h"

#define CLSNAME		_X("CSaveView_CIconView")

using namespace Framework;
using namespace boost;

PIXELFORMATDESCRIPTOR	CSaveView::CIconViewWnd::m_PFD =
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

CSaveView::CIconViewWnd::CIconViewWnd(HWND hParent, RECT* pR)
{
	m_nGrabbing = false;

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= NULL;
		wc.hbrBackground	= NULL; 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		wc.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&wc);
	}

	Create(WS_EX_STATICEDGE, CLSNAME, _X(""), WS_VISIBLE | WS_CLIPCHILDREN | WS_CHILD, pR, hParent, NULL);
	SetClassPtr();

	m_pThread = new thread(bind(&CSaveView::CIconViewWnd::ThreadProc, this));
}

CSaveView::CIconViewWnd::~CIconViewWnd()
{
	m_MailSlot.SendMessage(THREAD_END, NULL);
	m_pThread->join();
	delete m_pThread;
}

void CSaveView::CIconViewWnd::SetSave(const CSave* pSave)
{
	m_MailSlot.SendMessage(THREAD_SETSAVE, reinterpret_cast<void*>(const_cast<CSave*>(pSave)));
}

void CSaveView::CIconViewWnd::SetIconType(ICONTYPE nIconType)
{
	m_MailSlot.SendMessage(THREAD_SETICONTYPE, reinterpret_cast<void*>(&nIconType));
}

void CSaveView::CIconViewWnd::ThreadProc()
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
	
	m_pIconView = NULL;
	m_nIconType = ICON_NORMAL;
	m_nRotationX = 0;
	m_nRotationY = 0;
	m_nZoom = -7.0;

	nEnd = false;
	while(!nEnd)
	{
		if(m_MailSlot.IsMessagePending())
		{
			CThreadMsg::MESSAGE Message;
			m_MailSlot.GetMessage(&Message);
			switch(Message.nMsg)
			{
			case THREAD_SETSAVE:
				ThreadSetSave(reinterpret_cast<CSave*>(Message.pParam));
				break;
			case THREAD_SETICONTYPE:
				ThreadSetIconType(*reinterpret_cast<ICONTYPE*>(Message.pParam));
				break;
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

	DELETEPTR(m_pIconView);

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
}

long CSaveView::CIconViewWnd::OnLeftButtonDown(int nX, int nY)
{
	SetFocus();

	m_nGrabPosX = nX;
	m_nGrabPosY = nY;

	m_nGrabDistX = 0;
	m_nGrabDistY = 0;

	m_nGrabRotX = m_nRotationX;
	m_nGrabRotY = m_nRotationY;

	m_nGrabbing = true;
	SetCapture(m_hWnd);
	ChangeCursor();
	return TRUE;
}

long CSaveView::CIconViewWnd::OnLeftButtonUp(int nX, int nY)
{
	m_nGrabbing = false;
	ReleaseCapture();
	ChangeCursor();
	return TRUE;
}

long CSaveView::CIconViewWnd::OnMouseWheel(short nZ)
{
	if(nZ < 0)
	{
		m_nZoom -= 0.7;
	}
	else
	{
		m_nZoom += 0.7;
	}

	return TRUE;
}

long CSaveView::CIconViewWnd::OnMouseMove(WPARAM wParam, int nX, int nY)
{
	if(m_nGrabbing)
	{
		m_nGrabDistX = nX - m_nGrabPosX;
		m_nGrabDistY = nY - m_nGrabPosY;

		m_nRotationX = m_nGrabRotX + (double)m_nGrabDistY;
		m_nRotationY = m_nGrabRotY + (double)m_nGrabDistX;
	}
	return TRUE;	
}

long CSaveView::CIconViewWnd::OnSetCursor(HWND hWnd, unsigned int nX, unsigned int nY)
{
	ChangeCursor();
	return TRUE;
}

void CSaveView::CIconViewWnd::ThreadSetSave(const CSave* pSave)
{
	if(pSave == m_pSave) return;
	m_pSave = pSave;
	LoadIcon();
}

void CSaveView::CIconViewWnd::ThreadSetIconType(ICONTYPE nIconType)
{
	if(m_nIconType == nIconType) return;
	m_nIconType = nIconType;
	LoadIcon();
}

void CSaveView::CIconViewWnd::LoadIcon()
{
	DELETEPTR(m_pIconView);

	if(m_pSave == NULL) return;

	try
	{
		filesystem::path IconPath;

		switch(m_nIconType)
		{
		case ICON_NORMAL:
			IconPath = m_pSave->GetNormalIconPath();
			break;
		case ICON_DELETING:
			IconPath = m_pSave->GetDeletingIconPath();
			break;
		case ICON_COPYING:
			IconPath = m_pSave->GetCopyingIconPath();
			break;
		}

		m_pIconView = new CIconView(new CIcon(IconPath.string().c_str()));
	}
	catch(...)
	{

	}
}

void CSaveView::CIconViewWnd::ChangeCursor()
{
	if(m_nGrabbing)
	{
		SetCursor(LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_GRABBING)));
	}
	else
	{
		SetCursor(LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_GRAB)));
	}
}

void CSaveView::CIconViewWnd::Render(HDC hDC)
{
	RECT ClientRect;
	GetClientRect(&ClientRect);

	glViewport(0, 0, ClientRect.right, ClientRect.bottom);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawBackground();

	if(m_pIconView != NULL)
	{
		glEnable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, (float)ClientRect.right / (float)ClientRect.bottom, 0.1f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

//		glTranslated(0.0, -2.0, -7.0);
		glTranslated(0.0, 0.0, m_nZoom);
		glRotated(m_nRotationX, 1.0, 0.0, 0.0);
		glRotated(m_nRotationY, 0.0, 1.0, 0.0);
		glTranslated(0.0, -2.0, 0.0);
		glScaled(1.0, -1.0, -1.0);

		glColor4d(1.0, 1.0, 1.0, 1.0);

		m_pIconView->Render();
	}

	if(!m_nGrabbing)
	{
		m_nRotationY++;
	}

	SwapBuffers(hDC);
}

void CSaveView::CIconViewWnd::DrawBackground()
{
	glDisable(GL_DEPTH_TEST);

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
}
