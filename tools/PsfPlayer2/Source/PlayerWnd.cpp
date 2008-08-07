#include "PlayerWnd.h"
#include "PsfLoader.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "win32/AcceleratorTableGenerator.h"
#include "FileInformationWindow.h"
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
m_regView(NULL),
m_ready(false),
m_accel(CreateAccelerators())
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

	UpdateUi();

	m_virtualMachine.OnNewFrame.connect(bind(&CPlayerWnd::OnNewFrame, this));
}

CPlayerWnd::~CPlayerWnd()
{
	m_virtualMachine.Pause();
	delete m_regView;
}

void CPlayerWnd::Run()
{
    while(IsWindow())
    {
        MSG msg;
        GetMessage(&msg, 0, 0, 0);
        if(!TranslateAccelerator(m_hWnd, m_accel, &msg))
        {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
        }
    }
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
	case ID_FILE_PAUSE:
		PauseResume();
		break;
	case ID_FILE_FILEINFORMATION:
		ShowFileInformation();
		break;
	case ID_FILE_EXIT:
		Destroy();
		break;
	}
	return FALSE;
}

long CPlayerWnd::OnTimer()
{
//	TCHAR fps[32];
//	_stprintf(fps, _T("%i"), m_frames);
//	SetText(fps);
//	m_frames = 0;
	return FALSE;
}

HACCEL CPlayerWnd::CreateAccelerators()
{
	Win32::CAcceleratorTableGenerator tableGenerator;
	tableGenerator.Insert(ID_FILE_PAUSE, VK_F5, FVIRTKEY);
	return tableGenerator.Create();
}

void CPlayerWnd::PauseResume()
{
	if(!m_ready) return;
	if(m_virtualMachine.GetStatus() == CVirtualMachine::PAUSED)
	{
		m_virtualMachine.Resume();
	}
	else
	{
		m_virtualMachine.Pause();
	}
}

void CPlayerWnd::ShowFileInformation()
{
	if(!m_ready) return;
	CFileInformationWindow fileInfo(m_hWnd, m_tags);
	fileInfo.DoModal();
}

void CPlayerWnd::Load(const char* path)
{
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();
	m_tags.clear();
	CPsfLoader::LoadPsf(m_virtualMachine, path, &m_tags);
	m_virtualMachine.Resume();
	UpdateUi();
	m_ready = true;
}

void CPlayerWnd::UpdateUi()
{
	CPsfBase::ConstTagIterator titleTag = m_tags.find("title");
	bool hasTitle = titleTag != m_tags.end();

	tstring title = _T("PsfPlayer");
	if(hasTitle)
	{
		title += _T(" - [ ");
		title += string_cast<tstring>(titleTag->second);
		title += _T(" ]");
	}

	SetText(title.c_str());
}

void CPlayerWnd::OnNewFrame()
{
	m_regView->Render();
	m_regView->Redraw();
//	m_frames++;
}
