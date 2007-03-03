#ifndef _OUTPUTWND_H_
#define _OUTPUTWND_H_

#include "win32/Window.h"
#include "Event.h"

class COutputWnd : public Framework::Win32::CWindow
{
public:
									COutputWnd(HWND, RECT*);
									~COutputWnd();
	Framework::CEvent<int>			m_OnSizeChange;

protected:
	long							OnPaint();
	long							OnSize(unsigned int, unsigned int, unsigned int);

private:

};

#endif
