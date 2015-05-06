#pragma once

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
	enum
	{
		ID_MEMORYVIEWMIPS_GOTOADDRESS = ID_MEMORYVIEW_MENU_MAX,
		ID_MEMORYVIEWMIPS_FOLLOWPOINTER
	};

	uint8							GetByte(uint32) override;
	HMENU							CreateContextualMenu() override;

	long							OnCommand(unsigned short, unsigned short, HWND) override;

private:
	void							GotoAddress();
	void							FollowPointer();

	void							OnMachineStateChange();

	CMIPS*							m_context;
	CVirtualMachine&				m_virtualMachine;
};
