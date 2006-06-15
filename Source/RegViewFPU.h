#ifndef _REGVIEWFPU_H_
#define _REGVIEWFPU_H_

#include "RegViewPage.h"
#include "EventHandler.h"
#include "MIPS.h"

class CRegViewFPU : public CRegViewPage
{
public:
									CRegViewFPU(HWND, RECT*, CMIPS*);
									~CRegViewFPU();

	enum VIEWMODE
	{
		VIEWMODE_WORD   = 0,
		VIEWMODE_LONG   = 1,
		VIEWMODE_SINGLE = 2,
		VIEWMODE_DOUBLE = 3,
		VIEWMODE_MAX,
	};

protected:
	long							OnRightButtonUp(unsigned int, unsigned int);
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	void							Update();
	void							GetDisplayText(Framework::CStrA*);

	void							RenderWord(Framework::CStrA*);
	void							RenderSingle(Framework::CStrA*);
	void							RenderFCSR(Framework::CStrA*);

	void							OnMachineStateChange(int);
	void							OnRunningStateChange(int);

	VIEWMODE						m_nViewMode;
	CMIPS*							m_pCtx;
	Framework::CEventHandler<int>*	m_pOnMachineStateChangeHandler;
	Framework::CEventHandler<int>*	m_pOnRunningStateChangeHandler;
};

#endif
