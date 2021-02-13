#pragma once

// #include <QMdiArea>
#include <QWidget>
#include <QTableView>
#include "QtDisAsmTableModel.h"

#include "signal/Signal.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CDisAsmWnd : public QTableView, public CVirtualMachineStateView
{
public:
	typedef Framework::CSignal<void(uint32)> FindCallersRequestedEvent;

	CDisAsmWnd(QWidget*, CVirtualMachine&, CMIPS*, const char*, CQtDisAsmTableModel::DISASM_TYPE);
	virtual ~CDisAsmWnd() = default;

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

	void SetAddress(uint32);
	void SetCenterAtAddress(uint32);
	void SetSelectedAddress(uint32);

	FindCallersRequestedEvent FindCallersRequested;

protected:
	int sizeHintForColumn(int) const override;
	void verticalScrollbarValueChanged(int) override;

private:
	enum
	{
		HISTORY_STACK_MAX = 20
	};
	typedef std::pair<uint32, uint32> SelectionRangeType;

	int ComputeNumericalCellWidth() const;

	uint32 GetInstruction(uint32);

	void GotoAddress();
	void GotoPC();
	void GotoEA();
	void EditComment();
	void FindCallers();
	void ToggleBreakpoint(uint32);
	SelectionRangeType GetSelectionRange();
	void HistoryReset();
	void HistorySave(uint32);
	void HistoryGoBack();
	void HistoryGoForward();
	uint32 HistoryGetPrevious();
	uint32 HistoryGetNext();
	bool HistoryHasPrevious();
	bool HistoryHasNext();
	void ShowContextMenu(const QPoint&);
	void selectionChanged();
	void OnCopy();

	std::string GetInstructionDetailsText(uint32);
	std::string GetInstructionDetailsTextVu(uint32);

	void OnListDblClick();

	bool isAddressInView(QModelIndex& index) const;

	CVirtualMachine& m_virtualMachine;
	CMIPS* m_ctx;
	int32 m_instructionSize = 0;
	CQtDisAsmTableModel::DISASM_TYPE m_disAsmType;

	int m_numericalCellWidth = 0;
	uint32 m_address = 0;
	uint32 m_selected = MIPS_INVALID_PC;
	uint32 m_selectionEnd = -1;

	uint32 m_history[HISTORY_STACK_MAX];
	unsigned int m_historyPosition;
	unsigned int m_historySize;

	CQtDisAsmTableModel* m_model = nullptr;
};
