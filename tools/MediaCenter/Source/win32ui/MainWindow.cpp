#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "MainWindow.h"
#include "resource.h"
#include <boost/filesystem.hpp>

#define APP_NAME			_T("Play! Media Center")
#define WNDSTYLE			(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX)
#define WNDSTYLEEX			(0)

CMainWindow::CMainWindow() 
: m_frameBufferWindow(nullptr)
{
	Framework::Win32::CRect clientRect(0, 0, 640, 480);
	AdjustWindowRectEx(clientRect, WNDSTYLE, TRUE, WNDSTYLEEX);
	Create(WNDSTYLEEX, Framework::Win32::CDefaultWndClass::GetName(), APP_NAME, WNDSTYLE, clientRect, NULL, NULL);
	SetClassPtr();

	auto menu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAIN_MENU));
	SetMenu(menu);

	m_frameBufferWindow = new CFrameBufferWindow(m_hWnd);
	m_frameBufferWindow->SetViewportSize(640, 480);
}

CMainWindow::~CMainWindow()
{
	m_onNewFrameConnection.disconnect();
	delete m_frameBufferWindow;
}

long CMainWindow::OnCommand(unsigned short id, unsigned short msg, HWND hwndFrom)
{
	switch(id)
	{
	case ID_FILE_OPEN:
		OnFileOpen();
		break;
	case ID_FILE_QUIT:
		DestroyWindow(m_hWnd);
		break;
	}

	return TRUE;
}

void CMainWindow::OnNewFrame(const Framework::CBitmap& frame)
{
	m_frameBufferWindow->SetImage(frame.GetWidth(), frame.GetHeight(), frame.GetPixels());
}

void CMainWindow::OnFileOpen()
{
	Framework::Win32::CFileDialog dialog;
	dialog.m_OFN.lpstrFilter = _T("All Files (*.*)\0*.*\0");
	if(dialog.SummonOpen(m_hWnd))
	{
		boost::filesystem::path filePath(dialog.GetPath());
		auto myString = filePath.string();
		m_videoDecoder = std::make_shared<CVideoDecoder>(filePath.string());
		m_onNewFrameConnection = m_videoDecoder->NewFrame.connect([&] (const Framework::CBitmap& frame) { OnNewFrame(frame); });
/*
		boost::filesystem::path filePath(dialog.GetPath());
		std::wstring fileExtension = filePath.extension().wstring();
		if(!wcsicmp(fileExtension.c_str(), PLAYLIST_EXTENSION))
		{
			LoadPlaylist(filePath);
		}
		else if(!wcsicmp(fileExtension.c_str(), L".rar") || !wcsicmp(fileExtension.c_str(), L".zip"))
		{
			LoadArchive(filePath);
		}
		else
		{
			LoadSingleFile(filePath);
		}
*/
	}
}
