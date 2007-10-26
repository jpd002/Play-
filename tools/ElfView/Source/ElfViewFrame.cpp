#include "ElfViewFrame.h"
#include "win32/Rect.h"
#include "StdStream.h"

using namespace Framework;

#define CLSNAME _T("ElfViewFrame")

CElfViewFrame::CElfViewFrame(const char* path) :
m_elfView(NULL)
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
        Win32::CRect(0, 0, 770, 580), NULL, NULL);
	SetClassPtr();

	CreateClient(NULL);

    m_elfView = Open(path);
}

CElfViewFrame::~CElfViewFrame()
{
    if(m_elfView)
    {
        delete m_elfView;
    }
}

CElfViewEx* CElfViewFrame::Open(const char* path)
{
    CStdStream stream(path, "rb");    
    CElfViewEx* elfView = new CElfViewEx(m_pMDIClient->m_hWnd, stream);
    elfView->SetTextA(path);
    elfView->Show(SW_MAXIMIZE);
    return elfView;
}
