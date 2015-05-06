#ifndef _REGVIEWSCU_H_
#define _REGVIEWSCU_H_

#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewSCU : public CRegViewPage, public boost::signals2::trackable
{
public:
									CRegViewSCU(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewSCU();

private:
	void							Update();
	std::string						GetDisplayText();

	CMIPS*							m_pCtx;
};

#endif
