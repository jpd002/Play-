#pragma once

#include <QTableView>
#include "MemoryViewModel.h"
#include "signal/Signal.h"

class QResizeEvent;

class CMemoryViewTable : public QTableView
{
public:
	CMemoryViewTable(QWidget*);
	~CMemoryViewTable() = default;

	int GetBytesPerLine();
	void SetBytesPerLine(int);

	void SetData(CMemoryViewModel::getByteProto, uint64, uint32 = 0);
	void SetSelectionStart(uint32);

	void ShowEvent();
	void ResizeEvent();

	Framework::CSignal<void(uint32)> OnSelectionChange;

protected:
	int sizeHintForColumn(int) const override;

	CMemoryViewModel* m_model = nullptr;
	uint32 m_selected = 0;

private:
	int ComputeItemCellWidth() const;

	void ShowContextMenu(const QPoint&);
	void AutoColumn();
	void SetActiveUnit(int);
	void SelectionChanged();

	virtual void PopulateContextMenu(QMenu*){};

	int m_cwidth = 0;
	int m_bytesPerLine = 0;
	int m_maxUnits = 0;
};
