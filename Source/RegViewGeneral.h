#ifndef _REGVIEWGENERAL_H_
#define _REGVIEWGENERAL_H_

#include "RegViewPage.h"
#include "EventHandler.h"
#include "MIPS.h"

class CRegViewGeneral : public CRegViewPage
{
public:
									CRegViewGeneral(HWND, RECT*, CMIPS*);
									~CRegViewGeneral();

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
