#ifndef _SPUREGVIEWPANEL_H_
#define _SPUREGVIEWPANEL_H_

#include "win32/Window.h"
#include "iop/Iop_SpuBase.h"
#include "SpuRegView.h"

class CSpuRegViewPanel : public Framework::Win32::CWindow
{
public:
									CSpuRegViewPanel(HWND, const TCHAR*);
	virtual							~CSpuRegViewPanel();

	void							RefreshLayout();

	void							SetSpu(Iop::CSpuBase*);

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int);

private:
	CSpuRegView*					m_regView;
};

#endif
