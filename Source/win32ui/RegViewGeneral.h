#ifndef _REGVIEWGENERAL_H_
#define _REGVIEWGENERAL_H_

#include <boost/signal.hpp>
#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"
#include <string>

class CRegViewGeneral : public CRegViewPage, public boost::signals::trackable
{
public:
									CRegViewGeneral(HWND, RECT*, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewGeneral();

private:
	void							Update();
    std::string						GetDisplayText();

    CVirtualMachine&                m_virtualMachine;
    CMIPS*							m_pCtx;
};

#endif
