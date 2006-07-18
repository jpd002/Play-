#include "MemoryCardView.h"
#include "win32/DeviceContext.h"
#include <exception>
#include <gl/gl.h>
#include <gl/glu.h>
#include <math.h>

#define CLSNAME		_X("CMemoryCardView")

using namespace Framework;
using namespace std;

PIXELFORMATDESCRIPTOR	CMemoryCardView::m_PFD =
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

CMemoryCardView::CMemoryCardView(HWND hParent, RECT* pRect)
{
	unsigned int nPixelFormat;

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

	Create(WS_EX_CLIENTEDGE, CLSNAME, _X(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, pRect, hParent, NULL);
	SetClassPtr();

	m_hDC = GetDC(m_hWnd);
	nPixelFormat = ChoosePixelFormat(m_hDC, &m_PFD);
	SetPixelFormat(m_hDC, nPixelFormat, &m_PFD);
	m_hRC = wglCreateContext(m_hDC);
	wglMakeCurrent(m_hDC, m_hRC);

	UpdateGeometry();
	m_nRotation = 0;

	glEnable(GL_TEXTURE_2D);
	glClearColor(1.0, 1.0, 1.0, 1.0);

	SetTimer(m_hWnd, NULL, 16, NULL);
}

CMemoryCardView::~CMemoryCardView()
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
}

long CMemoryCardView::OnDrawItem(unsigned int nId, LPDRAWITEMSTRUCT pDrawItem)
{
	DrawScene();
	return TRUE;
}

void CMemoryCardView::SetMemoryCard(CMemoryCard* pMemoryCard)
{
	m_pMemoryCard = pMemoryCard;
	
	m_Saves.clear();
	m_Icons.clear();

	if(m_pMemoryCard != NULL)
	{
		for(CMemoryCard::SaveIterator itSave = m_pMemoryCard->GetSavesBegin();
			itSave != m_pMemoryCard->GetSavesEnd();
			itSave++)
		{
			try
			{
				m_Icons.push_back(new CIconView(new CIcon(itSave->GetNormalIconPath().string().c_str())));
				m_Saves.push_back(&(*itSave));
			}
			catch(...)
			{
				//Just ignore it.
			}
		}

		UpdateScroll();
	}

	SetSelection(0);
}

long CMemoryCardView::OnPaint()
{
	return TRUE;
}

long CMemoryCardView::OnTimer()
{
	Animate();
	DrawScene();
	return TRUE;
}

long CMemoryCardView::OnVScroll(unsigned int nType, unsigned int nThumb)
{
	switch(nType)
	{
	case SB_LINEDOWN:
		m_nScrollPosition += 5;
		break;
	case SB_LINEUP:
		m_nScrollPosition -= 5;
		break;
	case SB_PAGEDOWN:
		m_nScrollPosition += 35;
		break;
	case SB_PAGEUP:
		m_nScrollPosition -= 35;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		m_nScrollPosition = GetScrollThumbPosition(SB_VERT);
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
	SetSelection((nY + m_nScrollPosition) / m_nItemHeight);
	return TRUE;	
}

long CMemoryCardView::OnMouseWheel(short nZ)
{
	if(nZ < 0)
	{
		m_nScrollPosition += 35;
	}
	else
	{
		m_nScrollPosition -= 35;
	}

	UpdateScrollPosition();
	return TRUE;
}

long CMemoryCardView::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
	UpdateGeometry();
	return TRUE;
}

long CMemoryCardView::OnKeyDown(unsigned int nKey)
{
	switch(nKey)
	{
	case VK_DOWN:
		SetSelection(min<int>(m_nSelection + 1, (int)m_Icons.size() - 1));
		EnsureItemFullyVisible(m_nSelection);
		break;
	case VK_UP:
		SetSelection(max<int>(m_nSelection - 1, 0));
		EnsureItemFullyVisible(m_nSelection);
		break;
	}

	return TRUE;
}

void CMemoryCardView::Animate()
{
	for_each(m_Icons.begin(), m_Icons.end(), mem_fun_ref(&CIconView::Tick));
}

void CMemoryCardView::DrawScene()
{
	RECT ClientRect;
	int nY, nUpperClip, nLowerClip;

	glClear(GL_COLOR_BUFFER_BIT);

	GetClientRect(&ClientRect);

	nY = 0;
	nUpperClip = (int)(m_nScrollPosition - m_nItemHeight);
	nLowerClip = nUpperClip + ClientRect.bottom + m_nItemHeight;

	for(IconList::iterator itIcon = m_Icons.begin();
		itIcon != m_Icons.end();
		itIcon++)
	{
		if(nY >= nLowerClip) break;

		if(nY > nUpperClip)
		{
			glClear(GL_DEPTH_BUFFER_BIT);

			glViewport(0, (ClientRect.bottom - nY - m_nItemHeight + m_nScrollPosition), m_nItemWidth, m_nItemHeight);

			if(nY == (m_nSelection * m_nItemHeight))
			{
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, 1, 1, 0, 0, 1);

				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				glDisable(GL_DEPTH_TEST);

				glColor4d(0.0, 0.0, 0.0, 1.0);

				glBegin(GL_QUADS);
				{
					glVertex2d(0.0, 0.0);
					glVertex2d(1.0, 0.0);
					glVertex2d(1.0, 1.0);
					glVertex2d(0.0, 1.0);
				}
				glEnd();			
			}

			glEnable(GL_DEPTH_TEST);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glTranslated(0.0, -2.0, -7.0);
			glRotated(m_nRotation, 0.0, 1.0, 0.0);
			glScaled(1.0, -1.0, -1.0);

			glColor4d(1.0, 1.0, 1.0, 1.0);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(45.0f, (float)m_nItemWidth / (float)m_nItemHeight, 0.1f, 100.0f);

			itIcon->Render();

			//break;
		}

		nY += m_nItemHeight;
	}

	SwapBuffers(m_hDC);
}

void CMemoryCardView::UpdateScroll()
{
	SCROLLINFO si;

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nMin		= 0;
	si.nMax		= GetCanvasSize();
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);	
}

void CMemoryCardView::UpdateScrollPosition()
{
	SCROLLINFO si;

	m_nScrollPosition = max<int>(0, m_nScrollPosition);
	m_nScrollPosition = min<int>(GetCanvasSize(), m_nScrollPosition);

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= m_nScrollPosition;
	si.fMask		= SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
}

int CMemoryCardView::GetCanvasSize()
{
	RECT ClientRect;
	int nCanvasSize;

	GetClientRect(&ClientRect);
	nCanvasSize = ((int)m_Icons.size() * m_nItemHeight) - ClientRect.bottom;
	if(nCanvasSize < 0) nCanvasSize = 0;

	return nCanvasSize;
}

void CMemoryCardView::UpdateGeometry()
{
	RECT ClientRect;

	GetClientRect(&ClientRect);
	m_nItemHeight		= ClientRect.right;
	m_nItemWidth		= ClientRect.right;
	m_nSelection		= 0;
	m_nScrollPosition	= 0;

	UpdateScroll();
}

void CMemoryCardView::SetSelection(unsigned int nSelection)
{
	m_nSelection = nSelection;

	if(m_nSelection < m_Saves.size())
	{
		SaveList::iterator itSave;

		itSave = m_Saves.begin();
		for(unsigned int i = 0; i < m_nSelection; i++)
		{
			//There must be a better way to do that...
			itSave++;
		}

		m_OnSelectionChange.Notify(*itSave);
	}
	else
	{
		m_OnSelectionChange.Notify(NULL);
	}
}

void CMemoryCardView::EnsureItemFullyVisible(unsigned int nItem)
{
	int nWindowTop, nWindowBottom;
	int nItemTop, nItemBottom;
	RECT ClientRect;

	GetClientRect(&ClientRect);

	nWindowTop		= m_nScrollPosition;
	nWindowBottom	= m_nScrollPosition + ClientRect.bottom;

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

	UpdateScrollPosition();
}
