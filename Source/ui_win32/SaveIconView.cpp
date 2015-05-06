#include <boost/bind.hpp>
#include "resource.h"
#include "SaveIconView.h"
#include "StdStream.h"
#include "StdStreamUtils.h"
#include "win32/ClientDeviceContext.h"
#include "opengl/OpenGlDef.h"
#include "PtrMacro.h"

#define CLSNAME		_T("CSaveView_CIconView")

const PIXELFORMATDESCRIPTOR CSaveIconView::m_PFD =
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

CSaveIconView::CSaveIconView(HWND hParent, const RECT& rect)
: m_nGrabbing(false)
, m_iconMesh(NULL)
, m_iconType(CSave::ICON_NORMAL)
, m_nRotationX(0)
, m_nRotationY(0)
, m_nGrabPosX(0)
, m_nGrabPosY(0)
, m_nGrabDistX(0)
, m_nGrabDistY(0)
, m_nGrabRotX(0)
, m_nGrabRotY(0)
, m_nZoom(-7.0f)
, m_save(NULL)
, m_hRC(NULL)
, m_thread(NULL)
, m_threadOver(false)
{
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

	Create(WS_EX_STATICEDGE, CLSNAME, _T(""), WS_VISIBLE | WS_CLIPCHILDREN | WS_CHILD, rect, hParent, NULL);
	SetClassPtr();

	m_thread = new boost::thread(std::tr1::bind(&CSaveIconView::ThreadProc, this));
}

CSaveIconView::~CSaveIconView()
{
	m_threadOver = true;
	m_thread->join();
	delete m_thread;
}

void CSaveIconView::SetSave(const CSave* save)
{
	m_mailBox.SendCall(std::tr1::bind(&CSaveIconView::ThreadSetSave, this, save));
}

void CSaveIconView::SetIconType(CSave::ICONTYPE iconType)
{
	m_mailBox.SendCall(std::tr1::bind(&CSaveIconView::ThreadSetIconType, this, iconType));
}

void CSaveIconView::ThreadProc()
{
	Framework::Win32::CClientDeviceContext deviceContext(m_hWnd);

	unsigned int nPixelFormat = ChoosePixelFormat(deviceContext, &m_PFD);
	SetPixelFormat(deviceContext, nPixelFormat, &m_PFD);
	m_hRC = wglCreateContext(deviceContext);
	wglMakeCurrent(deviceContext, m_hRC);

	glEnable(GL_TEXTURE_2D);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	
	while(!m_threadOver)
	{
		while(m_mailBox.IsPending())
		{
            m_mailBox.ReceiveCall();
		}

		if(m_iconMesh)
		{
			m_iconMesh->Update(16.f / 1000.f);
		}
		Render(deviceContext);
		Sleep(16);
	}

	DELETEPTR(m_iconMesh);

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
}

long CSaveIconView::OnLeftButtonDown(int nX, int nY)
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

long CSaveIconView::OnLeftButtonUp(int nX, int nY)
{
	m_nGrabbing = false;
	ReleaseCapture();
	ChangeCursor();
	return TRUE;
}

long CSaveIconView::OnMouseWheel(int x, int y, short z)
{
	if(z < 0)
	{
		m_nZoom -= 0.7f;
	}
	else
	{
		m_nZoom += 0.7f;
	}

	return TRUE;
}

long CSaveIconView::OnMouseMove(WPARAM wParam, int nX, int nY)
{
	if(m_nGrabbing)
	{
		m_nGrabDistX = nX - m_nGrabPosX;
		m_nGrabDistY = nY - m_nGrabPosY;

		m_nRotationX = m_nGrabRotX + static_cast<float>(m_nGrabDistY);
		m_nRotationY = m_nGrabRotY + static_cast<float>(m_nGrabDistX);
	}
	return TRUE;	
}

long CSaveIconView::OnSetCursor(HWND hWnd, unsigned int nX, unsigned int nY)
{
	ChangeCursor();
	return TRUE;
}

void CSaveIconView::ThreadSetSave(const CSave* save)
{
	if(save == m_save) return;
	m_save = save;
	LoadIcon();
}

void CSaveIconView::ThreadSetIconType(CSave::ICONTYPE iconType)
{
	if(m_iconType == iconType) return;
	m_iconType = iconType;
	LoadIcon();
}

void CSaveIconView::LoadIcon()
{
	DELETEPTR(m_iconMesh);

	if(m_save == NULL) return;

	try
	{
		boost::filesystem::path iconPath = m_save->GetIconPath(m_iconType);

		auto iconStream(Framework::CreateInputStdStream(iconPath.native()));
		IconPtr icon(new CIcon(iconStream));
		m_iconMesh = new CIconMesh(icon);
	}
	catch(...)
	{

	}
}

void CSaveIconView::ChangeCursor()
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

void CSaveIconView::Render(HDC hDC)
{
	RECT clientRect = GetClientRect();

	glViewport(0, 0, clientRect.right, clientRect.bottom);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawBackground();

	if(m_iconMesh != NULL)
	{
		glEnable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, (float)clientRect.right / (float)clientRect.bottom, 0.1f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0.0, 0.0, m_nZoom);
		glRotatef(m_nRotationX, 1.0, 0.0, 0.0);
		glRotatef(m_nRotationY, 0.0, 1.0, 0.0);
		glTranslatef(0.0, -2.0, 0.0);
		glScalef(1.0, -1.0, -1.0);

		glColor4f(1.0, 1.0, 1.0, 1.0);

		m_iconMesh->Render();
	}

	if(!m_nGrabbing)
	{
		m_nRotationY++;
	}

	SwapBuffers(hDC);
}

void CSaveIconView::DrawBackground()
{
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBegin(GL_QUADS);
	{
		glColor4f(0.5, 0.5, 0.5, 1.0);
		glVertex2f(0.0, 0.0);
		glVertex2f(1.0, 0.0);

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glVertex2f(1.0, 0.5);
		glVertex2f(0.0, 0.5);

		glVertex2f(1.0, 0.5);
		glVertex2f(0.0, 0.5);

		glColor4f(0.5, 0.5, 0.5, 1.0);
		glVertex2f(0.0, 1.0);
		glVertex2f(1.0, 1.0);
	}
	glEnd();
}
