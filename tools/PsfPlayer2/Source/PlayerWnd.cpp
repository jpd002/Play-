#include "PlayerWnd.h"
#include "PsfLoader.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "string_cast.h"
#include "resource.h"
#include <afxres.h>
#include <functional>

#define CLSNAME		_T("PlayerWnd")
#define WNDSTYLE	(WS_CAPTION | WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(0)

using namespace Psx;
using namespace Framework;
using namespace std;
using namespace std::tr1;

CPlayerWnd::CPlayerWnd(CPsxVm& virtualMachine) :
m_virtualMachine(virtualMachine),
m_frames(0),
m_regView(NULL)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		RegisterClassEx(&Win32::CWindow::MakeWndClass(CLSNAME));
	}

	Create(WNDSTYLEEX, CLSNAME, _T("PsfPlayer"), WNDSTYLE, Win32::CRect(0, 0, 470, 570), NULL, NULL);
	SetClassPtr();

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINMENU)));

	SetTimer(m_hWnd, 0, 1000, NULL);

	m_regView = new CSpuRegView(m_hWnd, &GetClientRect(), m_virtualMachine.GetSpu());
	m_regView->Show(SW_SHOW);

	m_virtualMachine.OnNewFrame.connect(bind(&CPlayerWnd::OnNewFrame, this));
}

CPlayerWnd::~CPlayerWnd()
{
	m_virtualMachine.Pause();
	delete m_regView;
}

long CPlayerWnd::OnSize(unsigned int, unsigned int, unsigned int)
{
	if(m_regView != NULL)
	{
		RECT rect = GetClientRect();
		m_regView->SetSize(rect.right, rect.bottom);
	}
	return FALSE;
}

long CPlayerWnd::OnCommand(unsigned short id, unsigned short command, HWND hWndFrom)
{
	switch(id)
	{
	case ID_FILE_OPEN:
		{
			Win32::CFileDialog dialog;
			const TCHAR* filter = _T("PlayStation Sound Files (*.psf; *.minipsf)\0*.psf; *.minipsf\0");
			dialog.m_OFN.lpstrFilter = filter;
			if(dialog.Summon(m_hWnd))
			{
				Load(string_cast<string>(dialog.m_sFile).c_str());
			}
		}
		break;
	case ID_FILE_EXIT:
		Destroy();
		break;
	}
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

void CPlayerWnd::Load(const char* path)
{
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();
	CPsfLoader::LoadPsf(m_virtualMachine, path);
	m_virtualMachine.Resume();
}

void CPlayerWnd::OnNewFrame()
{
	m_regView->Render();
	m_regView->Redraw();
	m_frames++;
}
