#ifndef _SPUREGVIEW_H_
#define _SPUREGVIEW_H_

#include "win32ui/RegViewPage.h"
#include "Spu.h"

class CSpuRegView : public CRegViewPage
{
public:
					CSpuRegView(HWND, RECT*, CSpu&);
	virtual			~CSpuRegView();

	void			Render();

private:
	CSpu&			m_spu;
};

#endif
