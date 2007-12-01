#ifndef _REGVIEWVU_H_
#define _REGVIEWVU_H_

#include <boost/signal.hpp>
#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewVU : public CRegViewPage, public boost::signals::trackable
{
public:
									CRegViewVU(HWND, RECT*, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewVU();

private:
	void							Update();
	void							GetDisplayText(Framework::CStrA*);

	CMIPS*							m_pCtx;
};

#endif
