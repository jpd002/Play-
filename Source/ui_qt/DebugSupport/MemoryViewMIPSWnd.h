#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QLineEdit>

#include "MemoryViewTable.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"
#include "QtMemoryViewModel.h"

class QResizeEvent;

class CMemoryViewMIPSWnd : public QMdiSubWindow, public CVirtualMachineStateView
{
public:
	CMemoryViewMIPSWnd(QMdiArea*, CVirtualMachine&, CMIPS*, int);
	~CMemoryViewMIPSWnd();

	void HandleMachineStateChange() override;

	int GetBytesPerLine();
	void SetBytesPerLine(int);

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

private:
	void UpdateStatusBar(uint32);

	QLineEdit* m_addressEdit;
	CMemoryViewTable* m_tableView;

	Framework::CSignal<void(uint32)>::Connection m_OnSelectionChangeConnection;
};
