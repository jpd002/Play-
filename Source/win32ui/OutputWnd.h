#ifndef _OUTPUTWND_H_
#define _OUTPUTWND_H_

#include "win32/Window.h"
#include <boost/signal.hpp>

class COutputWnd : public Framework::Win32::CWindow
{
public:
									COutputWnd(HWND, RECT*);
									~COutputWnd();
    boost::signal<void ()>          m_OnSizeChange;

protected:
	long							OnPaint();
	long							OnSize(unsigned int, unsigned int, unsigned int);

private:

};

#endif
