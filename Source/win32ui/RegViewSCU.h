#ifndef _REGVIEWSCU_H_
#define _REGVIEWSCU_H_

#include "RegViewPage.h"
#include "EventHandler.h"
#include "../MIPS.h"

class CRegViewSCU : public CRegViewPage
{
public:
									CRegViewSCU(HWND, RECT*, CMIPS*);
									~CRegViewSCU();

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
