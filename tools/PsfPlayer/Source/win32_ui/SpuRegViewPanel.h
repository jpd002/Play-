#ifndef _SPUREGVIEWPANEL_H_
#define _SPUREGVIEWPANEL_H_

#include "win32/Dialog.h"
#include "iop/Iop_SpuBase.h"
#include "SpuRegView.h"

class CSpuRegViewPanel : public Framework::Win32::CDialog
{
public:
									CSpuRegViewPanel(HWND, const TCHAR*);
	virtual							~CSpuRegViewPanel();

	void							RefreshLayout();

	void							SetSpu(Iop::CSpuBase*);

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int) override;

private:
	CSpuRegView*					m_regView;
};

#endif
