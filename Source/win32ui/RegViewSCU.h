#ifndef _REGVIEWSCU_H_
#define _REGVIEWSCU_H_

#include <boost/signal.hpp>
#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewSCU : public CRegViewPage, public boost::signals::trackable
{
public:
									CRegViewSCU(HWND, RECT*, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewSCU();

private:
	void							Update();
	void							GetDisplayText(Framework::CStrA*);

	CMIPS*							m_pCtx;
};

#endif
