#include "AboutWindow.h"
#include "AppDef.h"
#include "resource.h"
#include <shobjidl.h>
#include <TDEmu.h>

CAboutWindow::CAboutWindow()
{
}

CAboutWindow::~CAboutWindow()
{
}

void CAboutWindow::DoModal(HWND parentWnd)
{
	std::tstring aboutMainInstruction;
	aboutMainInstruction = std::tstring(APP_NAME) + _T(" v") + std::tstring(APP_VERSIONSTR);

	std::tstring aboutContent;
	aboutContent += std::tstring(_T("By Jean-Philip Desjardins")) + _T("\r\n");
	aboutContent += std::tstring(_T("<a href=\"http://purei.org\">http://purei.org</a>")) + _T("\r\n");
	aboutContent += _T("\r\n");
	aboutContent += std::tstring(_T("Thanks to Neill Corlett for creating the PSF format and also for his ADSR and reverb analysis.")) + _T("\r\n");
	aboutContent += _T("\r\n");
	aboutContent += std::tstring(_T("Thanks to Koro from KoroSoft for his TaskDialog emulation library and his insights on various Win32 topics.")) + _T("\r\n");

	TASKDIALOGCONFIG taskDialog;
	memset(&taskDialog, 0, sizeof(TASKDIALOGCONFIG));
	taskDialog.cbSize = sizeof(TASKDIALOGCONFIG);
	taskDialog.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_ENABLE_HYPERLINKS;
	taskDialog.hInstance = GetModuleHandle(NULL);
	taskDialog.hwndParent = parentWnd;
	taskDialog.pszWindowTitle = _T("About");
	taskDialog.pszMainInstruction = aboutMainInstruction.c_str();
	taskDialog.pszContent = aboutContent.c_str();
	taskDialog.pszMainIcon = MAKEINTRESOURCE(IDI_MAIN);
	taskDialog.pfCallback = &CAboutWindow::TaskDialogCallback;
	TaskDialogIndirectEmulate(&taskDialog, NULL, NULL, NULL);
}

HRESULT CALLBACK CAboutWindow::TaskDialogCallback(HWND, UINT uNotification, WPARAM, LPARAM, LONG_PTR)
{
	if(uNotification == TDN_HYPERLINK_CLICKED)
	{
		ShellExecute(NULL, _T("open"), _T("http://purei.org"), NULL, NULL, SW_SHOW);
	}
	return S_OK;
}
