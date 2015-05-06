#ifndef _OUTPUTWND_H_
#define _OUTPUTWND_H_

#include "win32/Window.h"

class COutputWnd : public Framework::Win32::CWindow
{
public:
										COutputWnd(HWND);
	virtual								~COutputWnd();

protected:
	long								OnPaint() override;
	long								OnEraseBkgnd() override;

private:

};

#endif
