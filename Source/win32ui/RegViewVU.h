#ifndef _REGVIEWVU_H_
#define _REGVIEWVU_H_

#include "RegViewPage.h"
#include "EventHandler.h"
#include "../MIPS.h"

class CRegViewVU : public CRegViewPage
{
public:
									CRegViewVU(HWND, RECT*, CMIPS*);
									~CRegViewVU();

private:
	void							Update();
	void							GetDisplayText(Framework::CStrA*);

	void							OnMachineStateChange(int);
	void							OnRunningStateChange(int);

	CMIPS*							m_pCtx;

	Framework::CEventHandler<int>*	m_pOnMachineStateChangeHandler;
	Framework::CEventHandler<int>*	m_pOnRunningStateChangeHandler;
};

#endif
