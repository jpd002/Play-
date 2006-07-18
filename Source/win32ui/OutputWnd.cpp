#include "OutputWnd.h"
#include "../PS2VM.h"

#define CLSNAME		_X("COutputWnd")

using namespace Framework;

COutputWnd::COutputWnd(HWND hParent, RECT* pR)
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

	Create(NULL, CLSNAME, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, pR, hParent, NULL);
	SetClassPtr();
}

COutputWnd::~COutputWnd()
{

}

long COutputWnd::OnSize(unsigned int nMode, unsigned int nX, unsigned int nY)
{
	m_OnSizeChange.Notify(0);
	return TRUE;
}

long COutputWnd::OnPaint()
{
	if(CPS2VM::m_pGS != NULL)
	{
		CPS2VM::m_pGS->Flip();
	}
	return TRUE;
}
