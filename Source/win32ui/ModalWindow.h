#ifndef _MODALWINDOW_H_
#define _MODALWINDOW_H_

#include "win32/Window.h"

class CModalWindow : public Framework::CWindow
{
public:
					CModalWindow(HWND);
	virtual			~CModalWindow();
	void			DoModal();

protected:
	unsigned int	Destroy();
	long			OnSysCommand(unsigned int, LPARAM);
	void			UnModalWindow();
};

#endif
