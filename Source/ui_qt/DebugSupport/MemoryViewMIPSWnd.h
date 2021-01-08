#pragma once

#include <QWidget>
#include <QMdiSubWindow>
#include <QLineEdit>

#include "MemoryViewTable.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"
#include "QtMemoryViewModel.h"

class QResizeEvent;
namespace Ui
{
	class CMemoryViewMIPSWnd;
}

class CMemoryViewMIPSWnd : public QWidget, public CVirtualMachineStateView
{
public:
	CMemoryViewMIPSWnd(QWidget*, CVirtualMachine&, CMIPS*, int);
	~CMemoryViewMIPSWnd();

	void HandleMachineStateChange() override;

	int GetBytesPerLine();
	void SetBytesPerLine(int);

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

private:
	void UpdateStatusBar(uint32);

	Ui::CMemoryViewMIPSWnd* ui;

	Framework::CSignal<void(uint32)>::Connection m_OnSelectionChangeConnection;
};
