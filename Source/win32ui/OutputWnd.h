#ifndef _OUTPUTWND_H_
#define _OUTPUTWND_H_

#include "win32/Window.h"
#include <boost/signals2.hpp>

class COutputWnd : public Framework::Win32::CWindow
{
public:
										COutputWnd(HWND, RECT*);
										~COutputWnd();

	boost::signals2::signal<void ()>	OnSizeChange;

protected:
	long								OnPaint();
	long								OnSize(unsigned int, unsigned int, unsigned int);

private:

};

#endif
