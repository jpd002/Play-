#ifndef _SPUREGVIEWPANEL_H_
#define _SPUREGVIEWPANEL_H_

#include "Panel.h"
#include "iop/Iop_SpuBase.h"
#include "SpuRegView.h"

//#include "win32/Static.h"
//#include "win32/GdiObj.h"

class CSpuRegViewPanel : public CPanel
{
public:
									CSpuRegViewPanel(HWND);
	virtual							~CSpuRegViewPanel();

	void							RefreshLayout();

	void							SetSpu(Iop::CSpuBase*);

private:
	CSpuRegView*					m_regView;
};

#endif
