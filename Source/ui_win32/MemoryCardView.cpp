#include "MemoryCardView.h"
#include "win32/DeviceContext.h"
#include "StdStream.h"
#include "StdStreamUtils.h"
#include <boost/bind.hpp>
#include <exception>
#include <gl/gl.h>
#include <gl/glu.h>
#include <math.h>

#define CLSNAME		_T("CMemoryCardView")

const PIXELFORMATDESCRIPTOR CMemoryCardView::CRender::m_PFD =
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

CMemoryCardView::CMemoryCardView(HWND hParent, const RECT& rect)
: m_itemCount(0)
, m_memoryCard(NULL)
, m_render(NULL)
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

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, rect, hParent, NULL);
	SetClassPtr();

	UpdateGeometry();

	m_render = new CRender(m_hWnd, &m_viewState);
}

CMemoryCardView::~CMemoryCardView()
{
	delete m_render;
}

void CMemoryCardView::SetMemoryCard(CMemoryCard* memoryCard)
{
	m_memoryCard = memoryCard;
	
	if(m_memoryCard != NULL)
	{
		m_itemCount = static_cast<unsigned int>(m_memoryCard->GetSaveCount());
	}
	else
	{
		m_itemCount = 0;
	}

	m_viewState.m_nScrollPosition = 0;
	UpdateScroll();
	m_render->SetMemoryCard(memoryCard);
	SetSelection(0);
}

long CMemoryCardView::OnPaint()
{
	return TRUE;
}

long CMemoryCardView::OnVScroll(unsigned int nType, unsigned int nThumb)
{
	switch(nType)
	{
	case SB_LINEDOWN:
		m_viewState.m_nScrollPosition += 5;
		break;
	case SB_LINEUP:
		m_viewState.m_nScrollPosition -= 5;
		break;
	case SB_PAGEDOWN:
		m_viewState.m_nScrollPosition += 35;
		break;
	case SB_PAGEUP:
		m_viewState.m_nScrollPosition -= 35;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		m_viewState.m_nScrollPosition = GetVerticalScrollBar().GetThumbPosition();
		break;
	default:
		return FALSE;
		break;
	}

	UpdateScrollPosition();

	return TRUE;	
}

long CMemoryCardView::OnLeftButtonDown(int nX, int nY)
{
	SetFocus();
	SetSelection((nY + m_viewState.m_nScrollPosition) / m_viewState.m_nItemHeight);
	return TRUE;	
}

long CMemoryCardView::OnMouseWheel(int x, int y, short z)
{
	if(z < 0)
	{
		m_viewState.m_nScrollPosition += 35;
	}
	else
	{
		m_viewState.m_nScrollPosition -= 35;
	}

	UpdateScrollPosition();
	return TRUE;
}

long CMemoryCardView::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
	UpdateGeometry();
	return TRUE;
}

long CMemoryCardView::OnKeyDown(WPARAM nKey, LPARAM)
{
	switch(nKey)
	{
	case VK_DOWN:
		SetSelection(std::min<int>(m_viewState.m_nSelection + 1, m_itemCount - 1));
		EnsureItemFullyVisible(m_viewState.m_nSelection);
		break;
	case VK_UP:
		SetSelection(std::max<int>(m_viewState.m_nSelection - 1, 0));
		EnsureItemFullyVisible(m_viewState.m_nSelection);
		break;
	}

	return TRUE;
}

void CMemoryCardView::UpdateScroll()
{
	SCROLLINFO si;

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nMin		= 0;
	si.nMax		= m_viewState.GetCanvasSize(m_itemCount);
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);	
}

void CMemoryCardView::UpdateScrollPosition()
{
	m_viewState.m_nScrollPosition = std::max<int>(0,										m_viewState.m_nScrollPosition);
	m_viewState.m_nScrollPosition = std::min<int>(m_viewState.GetCanvasSize(m_itemCount),	m_viewState.m_nScrollPosition);

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= m_viewState.m_nScrollPosition;
	si.fMask		= SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
}

void CMemoryCardView::UpdateGeometry()
{
	RECT clientRect = GetClientRect();
	m_viewState.Reset(&clientRect);
	UpdateScroll();
}

void CMemoryCardView::SetSelection(unsigned int nSelection)
{
	m_viewState.m_nSelection = nSelection;

	if((m_viewState.m_nSelection < m_itemCount) && (m_memoryCard != NULL))
	{
		OnSelectionChange(m_memoryCard->GetSaveByIndex(m_viewState.m_nSelection));
	}
	else
	{
		OnSelectionChange(NULL);
	}
}

void CMemoryCardView::EnsureItemFullyVisible(unsigned int nItem)
{
	m_viewState.EnsureItemFullyVisible(nItem);
	UpdateScrollPosition();
}

//////////////////////////////////////
// CViewState implementation

int CMemoryCardView::CViewState::GetCanvasSize(unsigned int nItemCount)
{
	int nCanvasSize = (nItemCount * m_nItemHeight) - m_ClientRect.bottom;
	if(nCanvasSize < 0) nCanvasSize = 0;

	return nCanvasSize;
}

void CMemoryCardView::CViewState::Reset(RECT* pClientRect)
{
	CopyRect(&m_ClientRect, pClientRect);

	m_nItemHeight		= m_ClientRect.right;
	m_nItemWidth		= m_ClientRect.right;
	m_nSelection		= 0;
	m_nScrollPosition	= 0;
}

void CMemoryCardView::CViewState::EnsureItemFullyVisible(unsigned int nItem)
{
	int nWindowTop, nWindowBottom;
	int nItemTop, nItemBottom;

	nWindowTop		= m_nScrollPosition;
	nWindowBottom	= m_nScrollPosition + m_ClientRect.bottom;

	nItemTop		= nItem * m_nItemHeight;
	nItemBottom		= nItemTop + m_nItemHeight;
	
	if(nItemTop < nWindowTop)
	{
		m_nScrollPosition -= (nWindowTop - nItemTop);
	}
	else if(nItemBottom > nWindowBottom) 
	{
		m_nScrollPosition += (nItemBottom - nWindowBottom);
	}
}

//////////////////////////////////////
// CRender implementation

CMemoryCardView::CRender::CRender(HWND hWnd, const CViewState* pViewState) 
: m_deviceContext(hWnd)
, m_viewState(pViewState)
, m_memoryCard(NULL)
, m_threadOver(false)
, m_thread(NULL)
{
	m_thread = new boost::thread(std::tr1::bind(&CRender::ThreadProc, this));
}

CMemoryCardView::CRender::~CRender()
{
	m_threadOver = true;
	m_thread->join();
	delete m_thread;
}

void CMemoryCardView::CRender::ThreadProc()
{
	unsigned int nPixelFormat = ChoosePixelFormat(m_deviceContext, &m_PFD);
	SetPixelFormat(m_deviceContext, nPixelFormat, &m_PFD);
	m_hRC = wglCreateContext(m_deviceContext);
	wglMakeCurrent(m_deviceContext, m_hRC);

	glEnable(GL_TEXTURE_2D);
	glClearColor(1.0, 1.0, 1.0, 1.0);

	while(!m_threadOver)
	{
		while(m_mailBox.IsPending())
		{
			m_mailBox.ReceiveCall();
		}

		Animate();
		DrawScene();
		Sleep(16);
	}

	m_icons.clear();
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
}

void CMemoryCardView::CRender::SetMemoryCard(const CMemoryCard* memoryCard)
{
	m_mailBox.SendCall(std::tr1::bind(&CRender::ThreadSetMemoryCard, this, memoryCard));
}

void CMemoryCardView::CRender::ThreadSetMemoryCard(const CMemoryCard* memoryCard)
{
	m_icons.clear();
	m_memoryCard = memoryCard;
}

void CMemoryCardView::CRender::Animate()
{
	for(IconList::iterator iconIterator(m_icons.begin());
		m_icons.end() != iconIterator; ++iconIterator)
	{
		const IconMeshPtr& iconMesh(iconIterator->second);
		if(iconMesh)
		{
			iconMesh->Update(16.f / 1000.f);
		}
	}
}

void CMemoryCardView::CRender::DrawScene()
{
	RECT ClientRect;

	glClear(GL_COLOR_BUFFER_BIT);

	if(m_memoryCard == NULL) return;

	unsigned int nItemCount = static_cast<unsigned int>(m_memoryCard->GetSaveCount());
	CopyRect(&ClientRect, &m_viewState->m_ClientRect);

	int nY = 0;
	int nUpperClip = (int)(m_viewState->m_nScrollPosition - m_viewState->m_nItemHeight);
	int nLowerClip = nUpperClip + ClientRect.bottom + m_viewState->m_nItemHeight;

	for(unsigned int i = 0; i < nItemCount; i++)
	{
		if(nY >= nLowerClip) break;

		if(nY > nUpperClip)
		{
			glClear(GL_DEPTH_BUFFER_BIT);

			glViewport(0, 
				(ClientRect.bottom - nY - m_viewState->m_nItemHeight + m_viewState->m_nScrollPosition), 
				m_viewState->m_nItemWidth, 
				m_viewState->m_nItemHeight);

			if(nY == (m_viewState->m_nSelection * m_viewState->m_nItemHeight))
			{
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, 1, 1, 0, 0, 1);

				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				glDisable(GL_DEPTH_TEST);

				glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

				glBegin(GL_QUADS);
				{
					glVertex2f(0.0, 0.0);
					glVertex2f(1.0, 0.0);
					glVertex2f(1.0, 1.0);
					glVertex2f(0.0, 1.0);
				}
				glEnd();			
			}

			glEnable(GL_DEPTH_TEST);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glTranslatef(0.0f, -2.0f, -7.0f);
			glScalef(1.0f, -1.0f, -1.0f);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(45.0f, (float)m_viewState->m_nItemWidth / (float)m_viewState->m_nItemHeight, 0.1f, 100.0f);

			IconMeshPtr iconMesh;
			IconList::iterator itIcon = m_icons.find(i);
			if(itIcon == m_icons.end())
			{
				const CSave* pSave = m_memoryCard->GetSaveByIndex(i);
				try
				{
					auto iconStream(Framework::CreateInputStdStream(pSave->GetNormalIconPath().native()));
					IconPtr iconData(new CIcon(iconStream));
					iconMesh = IconMeshPtr(new CIconMesh(iconData));
				}
				catch(...)
				{

				}
				m_icons[i] = iconMesh;
			}
			else
			{
				iconMesh = itIcon->second;
			}

			if(iconMesh)
			{
				iconMesh->Render();
			}
		}

		nY += m_viewState->m_nItemHeight;
	}

	SwapBuffers(m_deviceContext);
}
