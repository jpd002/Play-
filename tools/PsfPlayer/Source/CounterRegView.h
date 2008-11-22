#ifndef _COUNTERREGVIEW_H_
#define _COUNTERREGVIEW_H_

#include "win32ui/RegViewPage.h"

class CCounterRegView : public CRegViewPage
{
public:
					CCounterRegView(HWND, RECT*);
	virtual			~CCounterRegView();

	virtual void	Render();

private:

};

#endif
