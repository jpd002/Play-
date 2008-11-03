#include "AppDef.h"
#include "PlayerWnd.h"
#include "PsfLoader.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "win32/AcceleratorTableGenerator.h"
#include "FileInformationWindow.h"
#include "AboutWindow.h"
#include "string_cast.h"
#include "resource.h"
#include <afxres.h>
#include <functional>

#define CLSNAME			_T("PlayerWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX)
#define WNDSTYLEEX		(0)
#define WM_UPDATEVIS	(WM_USER + 1)

using namespace Framework;
using namespace std;
using namespace std::tr1;

CPlayerWnd::CPlayerWnd(CPsfVm& virtualMachine) :
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

	Create(WNDSTYLEEX, CLSNAME, APP_NAME, WNDSTYLE, Win32::CRect(0, 0, 470, 580), NULL, NULL);
	SetClassPtr();

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINMENU)));

	SetTimer(m_hWnd, 0, 1000, NULL);

	m_regView = new CSpuRegView(m_hWnd, &GetClientRect(), m_virtualMachine.GetSpu());
	m_regView->Show(SW_SHOW);

	UpdateUi();
	SetIcon(ICON_SMALL, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN)));

	m_virtualMachine.OnNewFrame.connect(bind(&CPlayerWnd::OnNewFrame, this));
}

CPlayerWnd::~CPlayerWnd()
{
	m_virtualMachine.Pause();
	delete m_regView;
}

long CPlayerWnd::OnWndProc(unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
	case WM_UPDATEVIS:
		m_regView->Render();
		m_regView->Redraw();
		break;
	}
	return TRUE;
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
			const TCHAR* filter = 
				_T("All Supported Files\0*.psf; *.minipsf; *.psf2; *.minipsf2\0")
				_T("PlayStation Sound Files (*.psf; *.minipsf)\0*.psf; *.minipsf\0")
				_T("PlayStation2 Sound Files (*.psf2; *.minipsf2)\0*.psf2; *.minipsf2\0");
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
	case ID_HELP_ABOUT:
		ShowAbout();
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

void CPlayerWnd::ShowAbout()
{
	CAboutWindow about(m_hWnd);
	about.DoModal();
}

void CPlayerWnd::Load(const char* path)
{
	m_virtualMachine.Pause();
	m_virtualMachine.Reset();
	m_tags.clear();
	try
	{
		CPsfLoader::LoadPsf(m_virtualMachine, path, &m_tags);
		m_virtualMachine.Resume();
		m_ready = true;
	}
	catch(const exception& except)
	{
		tstring errorString = _T("Couldn't load PSF file: \r\n\r\n");
		errorString += string_cast<tstring>(except.what());
		MessageBox(m_hWnd, errorString.c_str(), NULL, 16);
		m_ready = false;
	}
	UpdateUi();
}

void CPlayerWnd::UpdateUi()
{
	CPsfBase::ConstTagIterator titleTag = m_tags.find("title");
	bool hasTitle = titleTag != m_tags.end();

	tstring title = APP_NAME;
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
	PostMessage(m_hWnd, WM_UPDATEVIS, 0, 0);
//	m_frames++;
}
