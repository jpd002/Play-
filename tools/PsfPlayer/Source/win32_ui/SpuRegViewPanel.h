#ifndef _SPUREGVIEWPANEL_H_
#define _SPUREGVIEWPANEL_H_

#include "Panel.h"
#include "iop/Iop_SpuBase.h"
#include "SpuRegView.h"

class CSpuRegViewPanel : public CPanel
{
public:
									CSpuRegViewPanel(HWND, const TCHAR*);
	virtual							~CSpuRegViewPanel();

	void							RefreshLayout();

	void							SetSpu(Iop::CSpuBase*);

private:
	CSpuRegView*					m_regView;
};

#endif
