#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include "QtGenericTableModel.h"

#include "DisAsm.h"
#include "VirtualMachineStateView.h"

class CDisAsmWnd : public QMdiSubWindow, public CVirtualMachineStateView
{
public:
	enum DISASM_TYPE
	{
		DISASM_STANDARD,
		DISASM_VU
	};

	CDisAsmWnd(QMdiArea*, CVirtualMachine&, CMIPS*, DISASM_TYPE);
	virtual ~CDisAsmWnd();

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

	CDisAsm* GetDisAsm() const;

// protected:
// 	long OnSize(unsigned int, unsigned int, unsigned int) override;
// 	long OnSysCommand(unsigned int, LPARAM) override;
// 	long OnSetFocus() override;

private:
	// void RefreshLayout();
	CDisAsm* m_disAsm = nullptr;
};
