#ifndef _MEMORYVIEWMIPS_H_
#define _MEMORYVIEWMIPS_H_

#include <boost/signal.hpp>
#include "MemoryView.h"
#include "../MIPS.h"

class CMemoryViewMIPS : public CMemoryView, public boost::signals::trackable
{
public:
									CMemoryViewMIPS(HWND, RECT*, CMIPS*);
	virtual							~CMemoryViewMIPS();

protected:
	virtual uint8					GetByte(uint32);
	virtual HFONT					GetFont();
	long							OnRightButtonUp(int, int);
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	void							GotoAddress();
	void							OnMachineStateChange();

	CMIPS*							m_pCtx;
};

#endif
