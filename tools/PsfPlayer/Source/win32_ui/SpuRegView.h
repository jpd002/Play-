#ifndef _SPUREGVIEW_H_
#define _SPUREGVIEW_H_

#include "win32ui/RegViewPage.h"
#include "iop/Iop_SpuBase.h"

class CSpuRegView : public CRegViewPage
{
public:
					CSpuRegView(HWND, RECT*, Iop::CSpuBase&);
	virtual			~CSpuRegView();

	void			Render();

private:
	Iop::CSpuBase&	m_spu;
};

#endif
