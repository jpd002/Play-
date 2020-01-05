#pragma once

#include <QWidget>
#include <QTableView>

#include "MIPS.h"
#include "VirtualMachineStateView.h"
#include "QtMemoryViewModel.h"
#include "Signal/Signal.h"

class QResizeEvent;

class CMemoryViewTable : public QTableView, public CVirtualMachineStateView
{
public:
	CMemoryViewTable(QWidget*, CVirtualMachine* = nullptr, CMIPS* = nullptr, int = 0, bool = false);
	~CMemoryViewTable();

	void HandleMachineStateChange() override;

	int GetBytesPerLine();
	void SetBytesPerLine(int);

	void SetData(CQtMemoryViewModel::getByteProto, int);

	void ShowEvent();
	void ResizeEvent();

	Framework::CSignal<void(uint32)> OnSelectionChange;

private:
	void ShowContextMenu(const QPoint&);
	void ResizeColumns();
	void AutoColumn();
	void GotoAddress();
	void FollowPointer();
	void SetActiveUnit(int);
	void SetSelectionStart(uint32);
	void SelectionChanged();

	CMIPS* m_context;
	CVirtualMachine* m_virtualMachine;
	CQtMemoryViewModel* m_model;

	uint32 m_selected = 0;
	int m_cwidth = 0;
	int m_bytesPerLine = 0;
	int m_maxUnits = 0;

	bool m_enableMemoryJumps;
};
