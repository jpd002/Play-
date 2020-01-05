#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include <QLineEdit>

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
	void UpdateStatusBar();
	void ShowContextMenu(const QPoint&);
	void ResizeColumns();
	void AutoColumn();
	void GotoAddress();
	void FollowPointer();
	void SetActiveUnit(int);
	void SetSelectionStart(uint32);
	void SelectionChanged();

	CMIPS* m_context;
	CVirtualMachine& m_virtualMachine;
	QLineEdit* m_addressEdit;
	QTableView* m_tableView;
	CQtMemoryViewModel* m_model;

	uint32 m_selected = 0;
	int m_cwidth = 0;
	int m_bytesPerLine = 0;
	int m_maxUnits = 0;

	Framework::CSignal<void(uint32)>::Connection m_OnSelectionChangeConnection;
};
