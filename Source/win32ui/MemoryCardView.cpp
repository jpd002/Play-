#include "MemoryCardView.h"
#include "win32/DeviceContext.h"
#include <boost/bind.hpp>
#include <exception>
#include <gl/gl.h>
#include <gl/glu.h>
#include <math.h>

#define CLSNAME		_T("CMemoryCardView")

using namespace Framework;
using namespace std;
using namespace boost;

PIXELFORMATDESCRIPTOR	CMemoryCardView::CRender::m_PFD =
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

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, pRect, hParent, NULL);
	SetClassPtr();

	UpdateGeometry();

	m_pThread = new thread(bind(&CMemoryCardView::ThreadProc, this));
}

CMemoryCardView::~CMemoryCardView()
{
	m_MailSlot.SendMessage(THREAD_END, NULL);
	m_pThread->join();
	delete m_pThread;
}

void CMemoryCardView::SetMemoryCard(CMemoryCard* pMemoryCard)
{
	m_pMemoryCard = pMemoryCard;
	
	if(m_pMemoryCard != NULL)
	{
		m_nItemCount = static_cast<unsigned int>(m_pMemoryCard->GetSaveCount());
	}
	else
	{
		m_nItemCount = 0;
	}

	m_ViewState.m_nScrollPosition = 0;
	UpdateScroll();
	m_MailSlot.SendMessage(THREAD_SETMEMORYCARD, pMemoryCard);

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
		m_ViewState.m_nScrollPosition += 5;
		break;
	case SB_LINEUP:
		m_ViewState.m_nScrollPosition -= 5;
		break;
	case SB_PAGEDOWN:
		m_ViewState.m_nScrollPosition += 35;
		break;
	case SB_PAGEUP:
		m_ViewState.m_nScrollPosition -= 35;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
        m_ViewState.m_nScrollPosition = GetVerticalScrollBar().GetThumbPosition();
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
	SetSelection((nY + m_ViewState.m_nScrollPosition) / m_ViewState.m_nItemHeight);
	return TRUE;	
}

long CMemoryCardView::OnMouseWheel(short nZ)
{
	if(nZ < 0)
	{
		m_ViewState.m_nScrollPosition += 35;
	}
	else
	{
		m_ViewState.m_nScrollPosition -= 35;
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
		SetSelection(min<int>(m_ViewState.m_nSelection + 1, m_nItemCount - 1));
		EnsureItemFullyVisible(m_ViewState.m_nSelection);
		break;
	case VK_UP:
		SetSelection(max<int>(m_ViewState.m_nSelection - 1, 0));
		EnsureItemFullyVisible(m_ViewState.m_nSelection);
		break;
	}

	return TRUE;
}

void CMemoryCardView::ThreadProc()
{
	CRender Render(m_hWnd, &m_ViewState);
	bool nIsEnd;

	nIsEnd = false;
	while(!nIsEnd)
	{
		CThreadMsg::MESSAGE Message;

		if(m_MailSlot.GetMessage(&Message))
		{
			switch(Message.nMsg)
			{
			case THREAD_END:
				nIsEnd = true;
				break;
			case THREAD_SETMEMORYCARD:
				Render.SetMemoryCard(reinterpret_cast<CMemoryCard*>(Message.pParam));
				break;
			}

			m_MailSlot.FlushMessage(0);
		}
		else
		{
			Render.Animate();
			Render.DrawScene();
			Sleep(16);
		}
	}
}

void CMemoryCardView::UpdateScroll()
{
	SCROLLINFO si;

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nMin		= 0;
	si.nMax		= m_ViewState.GetCanvasSize(m_nItemCount);
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);	
}

void CMemoryCardView::UpdateScrollPosition()
{
	SCROLLINFO si;

	m_ViewState.m_nScrollPosition = max<int>(0,											m_ViewState.m_nScrollPosition);
	m_ViewState.m_nScrollPosition = min<int>(m_ViewState.GetCanvasSize(m_nItemCount),	m_ViewState.m_nScrollPosition);

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= m_ViewState.m_nScrollPosition;
	si.fMask		= SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
}

void CMemoryCardView::UpdateGeometry()
{
	RECT ClientRect;
	GetClientRect(&ClientRect);

	m_ViewState.Reset(&ClientRect);
	UpdateScroll();
}

void CMemoryCardView::SetSelection(unsigned int nSelection)
{
	m_ViewState.m_nSelection = nSelection;

	if((m_ViewState.m_nSelection < m_nItemCount) && (m_pMemoryCard != NULL))
	{
		m_OnSelectionChange(m_pMemoryCard->GetSaveByIndex(m_ViewState.m_nSelection));
	}
	else
	{
		m_OnSelectionChange(NULL);
	}
}

void CMemoryCardView::EnsureItemFullyVisible(unsigned int nItem)
{
	m_ViewState.EnsureItemFullyVisible(nItem);
	UpdateScrollPosition();
}

//////////////////////////////////////
// CViewState implementation

int CMemoryCardView::CViewState::GetCanvasSize(unsigned int nItemCount)
{
	int nCanvasSize;

	nCanvasSize = (nItemCount * m_nItemHeight) - m_ClientRect.bottom;
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

CMemoryCardView::CRender::CRender(HWND hWnd, const CViewState* pViewState) :
m_DeviceContext(hWnd)
{
	unsigned int nPixelFormat;

	m_pViewState = pViewState;
	m_pMemoryCard = NULL;

	nPixelFormat = ChoosePixelFormat(m_DeviceContext, &m_PFD);
	SetPixelFormat(m_DeviceContext, nPixelFormat, &m_PFD);
	m_hRC = wglCreateContext(m_DeviceContext);
	wglMakeCurrent(m_DeviceContext, m_hRC);

	glEnable(GL_TEXTURE_2D);
	glClearColor(1.0, 1.0, 1.0, 1.0);
}

CMemoryCardView::CRender::~CRender()
{
	m_Icons.clear();
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
}

void CMemoryCardView::CRender::SetMemoryCard(const CMemoryCard* pMemoryCard)
{
	m_Icons.clear();
	m_pMemoryCard = pMemoryCard;
}

void CMemoryCardView::CRender::Animate()
{
	//for_each(m_Icons.begin(), m_Icons.end(), mem_fun_ref(&CIconView::Tick));
}

void CMemoryCardView::CRender::DrawScene()
{
	RECT ClientRect;
	int nY, nUpperClip, nLowerClip;
	unsigned int nItemCount;

	glClear(GL_COLOR_BUFFER_BIT);

	if(m_pMemoryCard == NULL) return;

	nItemCount = static_cast<unsigned int>(m_pMemoryCard->GetSaveCount());
	CopyRect(&ClientRect, &m_pViewState->m_ClientRect);

	nY = 0;
	nUpperClip = (int)(m_pViewState->m_nScrollPosition - m_pViewState->m_nItemHeight);
	nLowerClip = nUpperClip + ClientRect.bottom + m_pViewState->m_nItemHeight;

	for(unsigned int i = 0; i < nItemCount; i++)
	{
		if(nY >= nLowerClip) break;

		if(nY > nUpperClip)
		{
			IconList::iterator itIcon;
			CIconView* pIcon;

			glClear(GL_DEPTH_BUFFER_BIT);

			glViewport(0, 
				(ClientRect.bottom - nY - m_pViewState->m_nItemHeight + m_pViewState->m_nScrollPosition), 
				m_pViewState->m_nItemWidth, 
				m_pViewState->m_nItemHeight);

			if(nY == (m_pViewState->m_nSelection * m_pViewState->m_nItemHeight))
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
			glScaled(1.0, -1.0, -1.0);

			glColor4d(1.0, 1.0, 1.0, 1.0);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(45.0f, (float)m_pViewState->m_nItemWidth / (float)m_pViewState->m_nItemHeight, 0.1f, 100.0f);

			itIcon = m_Icons.find(i);
			if(itIcon == m_Icons.end())
			{
				const CSave* pSave;
				pSave = m_pMemoryCard->GetSaveByIndex(i);

				try
				{
					pIcon = new CIconView(new CIcon(pSave->GetNormalIconPath().string().c_str()));
					m_Icons.insert(i, pIcon);
				}
				catch(...)
				{

				}
			}
			else
			{
				pIcon = itIcon->second;
			}

			if(pIcon != NULL)
			{
				pIcon->Render();
			}
		}

		nY += m_pViewState->m_nItemHeight;
	}

	SwapBuffers(m_DeviceContext);
}
