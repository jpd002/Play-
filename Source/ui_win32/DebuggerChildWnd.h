#ifndef _DEBUGGERCHILDWND_H_
#define _DEBUGGERCHILDWND_H_

#include "win32/MDIChild.h"

class CDebuggerChildWnd : public Framework::Win32::CMDIChild
{
protected:
	virtual long    OnSysCommand(unsigned int, LPARAM);
};

#endif
