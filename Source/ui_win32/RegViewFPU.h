#ifndef _REGVIEWFPU_H_
#define _REGVIEWFPU_H_

#include <string>
#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewFPU : public CRegViewPage, public boost::signals2::trackable
{
public:
									CRegViewFPU(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewFPU();

protected:
	long							OnRightButtonUp(int, int);
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	enum VIEWMODE
	{
		VIEWMODE_WORD,
		VIEWMODE_SINGLE,
		VIEWMODE_MAX,
	};

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
