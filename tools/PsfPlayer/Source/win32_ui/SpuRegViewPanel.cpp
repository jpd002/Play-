#include "SpuRegViewPanel.h"
#include "win32/Rect.h"
#include <boost/lexical_cast.hpp>

#define CLSNAME			                _T("PlaylistPanel")
#define WNDSTYLE		                (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CHILD)
#define WNDSTYLEEX		                (0)

CSpuRegViewPanel::CSpuRegViewPanel(HWND parentWnd, const TCHAR* title)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&Framework::Win32::CWindow::MakeWndClass(CLSNAME));
	}

	Create(WNDSTYLEEX, CLSNAME, _T(""), WNDSTYLE, Framework::Win32::CRect(0, 0, 1, 1), parentWnd, NULL);
    SetClassPtr();

	m_regView = new CSpuRegView(m_hWnd, title);
}

CSpuRegViewPanel::~CSpuRegViewPanel()
{
	delete m_regView;
}

void CSpuRegViewPanel::SetSpu(Iop::CSpuBase* spu)
{
	m_regView->SetSpu(spu);
}

void CSpuRegViewPanel::RefreshLayout()
{
    //Resize panel
    {
		Framework::Win32::CRect clientRect(0, 0, 0, 0);
        ::GetClientRect(GetParent(), clientRect);
        SetSizePosition(clientRect);
    }

	//Resize view
	{
		Framework::Win32::CRect clientRect(GetClientRect());
		m_regView->SetSizePosition(clientRect);
	}
}
