#pragma once

#include "MemoryViewTable.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CMemoryViewMIPS : public CMemoryViewTable, public CVirtualMachineStateView
{
public:
	CMemoryViewMIPS(QWidget*);
	~CMemoryViewMIPS() = default;

	void Setup(CVirtualMachine* = nullptr, CMIPS* = nullptr, bool = false);

	void HandleMachineStateChange() override;
	void SetAddress(uint32);

private:
	void PopulateContextMenu(QMenu*) override;

	void GotoAddress();
	void FollowPointer();

	CMIPS* m_context = nullptr;
	CVirtualMachine* m_virtualMachine = nullptr;
};
