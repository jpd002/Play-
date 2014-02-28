#ifndef _MEMORYVIEWMIPS_H_
#define _MEMORYVIEWMIPS_H_

#include <boost/signals2.hpp>
#include "../MIPS.h"
#include "../VirtualMachine.h"
#include "MemoryView.h"

class CMemoryViewMIPS : public CMemoryView, public boost::signals2::trackable
{
public:
									CMemoryViewMIPS(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CMemoryViewMIPS();

protected:
	virtual uint8					GetByte(uint32);
	virtual HFONT					GetFont();
	long							OnRightButtonUp(int, int);
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	void							GotoAddress();
	void							FollowPointer();

	void							OnMachineStateChange();

	CMIPS*							m_pCtx;
	CVirtualMachine&				m_virtualMachine;
};

#endif
