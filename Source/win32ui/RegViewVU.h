#ifndef _REGVIEWVU_H_
#define _REGVIEWVU_H_

#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewVU : public CRegViewPage, public boost::signals2::trackable
{
public:
									CRegViewVU(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewVU();

private:
	void							Update();
	std::string						GetDisplayText();

	CMIPS*							m_pCtx;
};

#endif
