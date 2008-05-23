#include "PlayerWnd.h"
#include "win32/Rect.h"
#include <functional>

#define CLSNAME		_T("PlayerWnd")
#define WNDSTYLE	(WS_CAPTION | WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(0)

using namespace Framework;
using namespace std::tr1;

CPlayerWnd::CPlayerWnd(CPsxVm& virtualMachine) :
m_virtualMachine(virtualMachine),
m_frames(0)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&Win32::CWindow::MakeWndClass(CLSNAME));
	}

    Create(WNDSTYLEEX, CLSNAME, _T("PsfPlayer"), WNDSTYLE, Win32::CRect(0, 0, 470, 550), NULL, NULL);
	SetClassPtr();

	SetTimer(m_hWnd, 0, 1000, NULL);

	m_regView = new CSpuRegView(m_hWnd, &GetClientRect(), m_virtualMachine.GetSpu());
	m_regView->Show(SW_SHOW);

	m_virtualMachine.OnNewFrame.connect(bind(&CPlayerWnd::OnNewFrame, this));
	m_virtualMachine.Resume();
}

CPlayerWnd::~CPlayerWnd()
{
	m_virtualMachine.Pause();
	delete m_regView;
}

long CPlayerWnd::OnSize(unsigned int, unsigned int, unsigned int)
{
	RECT rect = GetClientRect();
	m_regView->SetSize(rect.right, rect.bottom);
	return FALSE;
}

long CPlayerWnd::OnTimer()
{
	TCHAR fps[32];
	_stprintf(fps, _T("%i"), m_frames);
	SetText(fps);
	m_frames = 0;
	return FALSE;
}

void CPlayerWnd::OnNewFrame()
{
	m_regView->Render();
	m_regView->Redraw();
	m_frames++;
}
