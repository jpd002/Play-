#include "OutputWnd.h"
#include "../PS2VM.h"

#define CLSNAME		_T("COutputWnd")
#define WNDSTYLE	(WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)

COutputWnd::COutputWnd(HWND hParent)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		wc.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&wc);
	}

	Create(NULL, CLSNAME, NULL, WNDSTYLE, Framework::Win32::CRect(0, 0, 1, 1), hParent, NULL);
	SetClassPtr();
}

COutputWnd::~COutputWnd()
{

}

long COutputWnd::OnEraseBkgnd()
{
	return TRUE;
}

long COutputWnd::OnPaint()
{
	return TRUE;
}
