#ifndef _MEMORYVIEWMIPS_H_
#define _MEMORYVIEWMIPS_H_

#include <boost/signal.hpp>
#include "../MIPS.h"
#include "../VirtualMachine.h"
#include "MemoryView.h"

class CMemoryViewMIPS : public CMemoryView, public boost::signals::trackable
{
public:
									CMemoryViewMIPS(HWND, RECT*, CVirtualMachine&, CMIPS*);
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
    CVirtualMachine&                m_virtualMachine;
};

#endif
