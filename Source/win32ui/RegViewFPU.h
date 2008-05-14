#ifndef _REGVIEWFPU_H_
#define _REGVIEWFPU_H_

#include <boost/signal.hpp>
#include <string>
#include "RegViewPage.h"
#include "EventHandler.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewFPU : public CRegViewPage, public boost::signals::trackable
{
public:
									CRegViewFPU(HWND, RECT*, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewFPU();

	enum VIEWMODE
	{
		VIEWMODE_WORD   = 0,
		VIEWMODE_LONG   = 1,
		VIEWMODE_SINGLE = 2,
		VIEWMODE_DOUBLE = 3,
		VIEWMODE_MAX,
	};

protected:
	long							OnRightButtonUp(int, int);
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	void							Update();
    std::string 					GetDisplayText();

    std::string						RenderWord();
    std::string						RenderSingle();
    std::string						RenderFCSR();

	void							OnMachineStateChange();
	void							OnRunningStateChange();

	VIEWMODE						m_nViewMode;
	CMIPS*							m_pCtx;
};

#endif
