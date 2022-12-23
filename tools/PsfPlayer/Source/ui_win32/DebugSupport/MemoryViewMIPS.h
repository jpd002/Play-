#pragma once

#include "MIPS.h"
#include "VirtualMachineStateView.h"
#include "MemoryView.h"

class CMemoryViewMIPS : public CMemoryView, public CVirtualMachineStateView
{
public:
	CMemoryViewMIPS(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual ~CMemoryViewMIPS() = default;

	void HandleMachineStateChange() override;

protected:
	enum
	{
		ID_MEMORYVIEWMIPS_GOTOADDRESS = ID_MEMORYVIEW_MENU_MAX,
		ID_MEMORYVIEWMIPS_FOLLOWPOINTER
	};

	uint8 GetByte(uint32) override;
	HMENU CreateContextualMenu() override;

	long OnCommand(unsigned short, unsigned short, HWND) override;

private:
	void GotoAddress();
	void FollowPointer();

	void OnMachineStateChange();

	CMIPS* m_context;
	CVirtualMachine& m_virtualMachine;
};
