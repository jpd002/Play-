#pragma once

#include <QWidget>
#include <QTableView>

#include "MIPS.h"
#include "VirtualMachineStateView.h"
#include "QtMemoryViewModel.h"
#include "signal/Signal.h"

class QResizeEvent;

class CMemoryViewTable : public QTableView, public CVirtualMachineStateView
{
public:
	CMemoryViewTable(QWidget*);
	~CMemoryViewTable() = default;

	void Setup(CVirtualMachine* = nullptr, CMIPS* = nullptr, bool = false);

	void HandleMachineStateChange() override;

	int GetBytesPerLine();
	void SetBytesPerLine(int);

	void SetData(CQtMemoryViewModel::getByteProto, int);

	void ShowEvent();
	void ResizeEvent();

	Framework::CSignal<void(uint32)> OnSelectionChange;

protected:
	int sizeHintForColumn(int) const override;

private:
	int ComputeItemCellWidth() const;

	void ShowContextMenu(const QPoint&);
	void AutoColumn();
	void GotoAddress();
	void FollowPointer();
	void SetActiveUnit(int);
	void SetSelectionStart(uint32);
	void SelectionChanged();

	CMIPS* m_context = nullptr;
	CVirtualMachine* m_virtualMachine = nullptr;
	CQtMemoryViewModel* m_model = nullptr;

	uint32 m_selected = 0;
	int m_cwidth = 0;
	int m_bytesPerLine = 0;
	int m_maxUnits = 0;

	bool m_enableMemoryJumps = false;
};
