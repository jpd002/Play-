#ifndef _REGVIEWGENERAL_H_
#define _REGVIEWGENERAL_H_

#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"
#include <string>

class CRegViewGeneral : public CRegViewPage, public boost::signals2::trackable
{
public:
									CRegViewGeneral(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewGeneral();

private:
	void							Update();
	std::string						GetDisplayText();

	CVirtualMachine&				m_virtualMachine;
	CMIPS*							m_pCtx;
};

#endif
