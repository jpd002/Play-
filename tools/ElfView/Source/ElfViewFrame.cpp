#include "ElfViewFrame.h"
#include "win32/Rect.h"
#include "win32/FileDialog.h"
#include "StdStream.h"
#include "resource.h"
#include "string_cast.h"
#include "AppDef.h"

using namespace Framework;

#define CLSNAME _T("ElfViewFrame")

CElfViewFrame::CElfViewFrame(const char* path)
    : m_elfView(NULL)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = CLSNAME;
		wc.lpfnWndProc = CWindow::WndProc;
		RegisterClassEx(&wc);
	}

	Create(NULL, CLSNAME, APP_NAME, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	       Win32::CRect(0, 0, 770, 580), NULL, NULL);
	SetClassPtr();

	CreateClient(NULL);

	{
		int smallIconSizeX = GetSystemMetrics(SM_CXSMICON);
		int smallIconSizeY = GetSystemMetrics(SM_CYSMICON);
		int bigIconSizeX = GetSystemMetrics(SM_CXICON);
		int bigIconSizeY = GetSystemMetrics(SM_CYICON);

		HICON smallIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, smallIconSizeX, smallIconSizeY, 0));
		HICON bigIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, bigIconSizeX, bigIconSizeY, 0));

		SetIcon(ICON_SMALL, smallIcon);
		SetIcon(ICON_BIG, bigIcon);
	}

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINMENU)));

	if(strlen(path) != 0)
	{
		Load(path);
	}
}

CElfViewFrame::~CElfViewFrame()
{
	CloseCurrentFile();
}

long CElfViewFrame::OnCommand(unsigned short id, unsigned short msg, HWND hwndFrom)
{
	switch(id)
	{
	case ID_FILE_OPENELF:
		OpenElf();
		break;
	case ID_FILE_QUIT:
		Destroy();
		break;
	case ID_HELP_ABOUT:
		ShowAboutBox();
		break;
	}
	return TRUE;
}

void CElfViewFrame::OpenElf()
{
	Win32::CFileDialog d;
	d.m_OFN.lpstrFilter = _T("ELF Executable Files (*.elf; *.so)\0*.elf; *.so\0All files (*.*)\0*.*\0");

	Enable(FALSE);
	int nRet = d.SummonOpen(m_hWnd);
	Enable(TRUE);
	SetFocus();

	if(nRet == 0) return;

	Load(string_cast<std::string>(d.GetPath()).c_str());
}

void CElfViewFrame::Load(const char* path)
{
	CloseCurrentFile();

	try
	{
		CStdStream stream(path, "rb");
		m_elfView = new CElfViewEx(m_pMDIClient->m_hWnd);
		m_elfView->LoadElf(stream);
		m_elfView->SetTextA(path);
		m_elfView->Show(SW_MAXIMIZE);
	}
	catch(const std::exception& except)
	{
		CloseCurrentFile();
		MessageBoxA(m_hWnd, except.what(), NULL, 16);
	}
}

void CElfViewFrame::CloseCurrentFile()
{
	if(m_elfView)
	{
		m_elfView->Show(SW_HIDE);
		m_pMDIClient->DestroyChild(m_elfView->m_hWnd);
		m_elfView->Destroy();
		delete m_elfView;
		m_elfView = NULL;
	}
}

#define MAIL_HYPERLINK_NAME _T("mail")
#define MAIL_HYPERLINK_VALUE _T("jean-philip.desjardins@polymtl.ca")

#define WEBSITE_HYPERLINK_NAME _T("website")
#define WEBSITE_HYPERLINK_VALUE _T("http://purei.org")

HRESULT CALLBACK CElfViewFrame::TaskDialogCallback(HWND, UINT notification, WPARAM wParam, LPARAM lParam, LONG_PTR)
{
	switch(notification)
	{
	case TDN_HYPERLINK_CLICKED:
	{
		WCHAR* hyperlinkName = reinterpret_cast<WCHAR*>(lParam);
		if(!wcscmp(hyperlinkName, MAIL_HYPERLINK_NAME))
		{
			ShellExecute(NULL, _T("open"), _T("mailto:") MAIL_HYPERLINK_VALUE, NULL, NULL, SW_SHOW);
		}
		else if(!wcscmp(hyperlinkName, WEBSITE_HYPERLINK_NAME))
		{
			ShellExecute(NULL, _T("open"), WEBSITE_HYPERLINK_VALUE, NULL, NULL, SW_SHOW);
		}
	}
	break;
	}
	return S_OK;
}

void CElfViewFrame::ShowAboutBox()
{
	typedef HRESULT(STDAPICALLTYPE * TaskDialogIndirectFunction)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);

	std::tstring windowTitle = _T("About ") + std::tstring(APP_NAME);
	std::tstring versionMessage = std::tstring(APP_NAME) + _T(" v") + std::tstring(APP_VERSIONSTR);

	bool succeeded = false;
	//if(false)
	{
		HMODULE comctl32Lib = LoadLibrary(_T("comctl32.dll"));
		if(comctl32Lib != NULL)
		{
			TaskDialogIndirectFunction taskDialogIndirect(NULL);
			taskDialogIndirect = reinterpret_cast<TaskDialogIndirectFunction>(GetProcAddress(comctl32Lib, "TaskDialogIndirect"));
			if(taskDialogIndirect != NULL)
			{
				TASKDIALOGCONFIG dialogConfig;
				memset(&dialogConfig, 0, sizeof(dialogConfig));
				dialogConfig.cbSize = sizeof(dialogConfig);
				dialogConfig.hwndParent = m_hWnd;
				dialogConfig.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
				dialogConfig.pszWindowTitle = windowTitle.c_str();
				dialogConfig.pszContent = _T("Written by Jean-Philip Desjardins\r\n\r\nE-mail: <a href=\"") MAIL_HYPERLINK_NAME _T("\">") MAIL_HYPERLINK_VALUE _T("</a>\r\nOfficial Website: <a href=\"") WEBSITE_HYPERLINK_NAME _T("\">") WEBSITE_HYPERLINK_VALUE _T("</a>");
				dialogConfig.pszMainInstruction = versionMessage.c_str();
				dialogConfig.pfCallback = &CElfViewFrame::TaskDialogCallback;
				taskDialogIndirect(&dialogConfig, NULL, NULL, NULL);
				succeeded = true;
			}
			FreeLibrary(comctl32Lib);
		}
	}

	if(!succeeded)
	{
		std::tstring message;
		message += versionMessage + _T("\r\n\r\n");
		message += _T("Written by Jean-Philip Desjardins\r\n\r\n");
		message += _T("E-mail: ") MAIL_HYPERLINK_VALUE _T("\r\n");
		message += _T("Official Website: ") WEBSITE_HYPERLINK_VALUE;
		MessageBox(m_hWnd, message.c_str(), windowTitle.c_str(), 0);
	}
}
