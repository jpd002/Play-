#ifndef _REGVIEWGENERAL_H_
#define _REGVIEWGENERAL_H_

#include <boost/signal.hpp>
#include "RegViewPage.h"
#include "../MIPS.h"

class CRegViewGeneral : public CRegViewPage, public boost::signals::trackable
{
public:
									CRegViewGeneral(HWND, RECT*, CMIPS*);
	virtual							~CRegViewGeneral();

private:
	void							Update();
	void							GetDisplayText(Framework::CStrA*);

	CMIPS*							m_pCtx;
};

#endif
