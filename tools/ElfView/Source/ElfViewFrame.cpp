#include "ElfViewFrame.h"
#include "win32/Rect.h"

using namespace Framework;

#define CLSNAME _T("ElfViewFrame")

CElfViewFrame::CElfViewFrame()
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)GetStockObject(GRAY_BRUSH); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		RegisterClassEx(&wc);
	}
	
    Create(NULL, CLSNAME, _T("Elf Viewer"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 
        Win32::CRect(0, 0, 640, 480), NULL, NULL);
	SetClassPtr();

//	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_DEBUGGER)));

	CreateClient(NULL);
}

CElfViewFrame::~CElfViewFrame()
{

}

