#ifndef _PANEL_H_
#define _PANEL_H_

#include "win32/Window.h"

class CPanel : public Framework::Win32::CWindow
{
public:
    virtual                 ~CPanel() {}
    virtual void            RefreshLayout() = 0;

private:

};

#endif
