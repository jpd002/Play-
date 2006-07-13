#ifndef _MEMORYVIEWMIPS_H_
#define _MEMORYVIEWMIPS_H_

#include "MemoryView.h"
#include "../MIPS.h"
#include "EventHandler.h"

class CMemoryViewMIPS : public CMemoryView
{
public:
									CMemoryViewMIPS(HWND, RECT*, CMIPS*);
									~CMemoryViewMIPS();

protected:
	virtual uint8					GetByte(uint32);
	virtual HFONT					GetFont();
	long							OnRightButtonUp(int, int);
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	void							GotoAddress();
	void							OnMachineStateChange(int);

	Framework::CEventHandler<int>*	m_pOnMachineStateChangeHandler;

	CMIPS*							m_pCtx;
};

#endif
