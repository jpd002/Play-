#pragma once

#include <QWidget>
#include "MIPS.h"
#include "VirtualMachineStateView.h"

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

	void SetAddress(uint32);

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

private:
	void UpdateStatusBar(uint32);

	Ui::CMemoryViewMIPSWnd* ui;

	Framework::CSignal<void(uint32)>::Connection m_OnSelectionChangeConnection;
};
