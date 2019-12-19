#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include "QtDisAsmTableModel.h"

#include "signal/Signal.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CDisAsmWnd : public QMdiSubWindow, public CVirtualMachineStateView
{
public:
	typedef Framework::CSignal<void(uint32)> FindCallersRequestedEvent;

	CDisAsmWnd(QMdiArea*, CVirtualMachine&, CMIPS*, const char*, CQtDisAsmTableModel::DISASM_TYPE);
	virtual ~CDisAsmWnd();

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

	void SetAddress(uint32);
	void SetCenterAtAddress(uint32);
	void SetSelectedAddress(uint32);

	FindCallersRequestedEvent FindCallersRequested;

private:
	enum
	{
		HISTORY_STACK_MAX = 20
	};
	typedef std::pair<uint32, uint32> SelectionRangeType;

	uint32 GetInstruction(uint32);

	void GotoAddress();
	void GotoPC();
	void GotoEA();
	void EditComment();
	void FindCallers();
	void UpdatePosition(int);
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

	CVirtualMachine& m_virtualMachine;
	CMIPS* m_ctx;
	int32 m_instructionSize;
	CQtDisAsmTableModel::DISASM_TYPE m_disAsmType;

	uint32 m_address;
	uint32 m_selected;
	uint32 m_selectionEnd;
	bool m_focus;

	uint32 m_history[HISTORY_STACK_MAX];
	unsigned int m_historyPosition;
	unsigned int m_historySize;

	QTableView* m_tableView;
	CQtDisAsmTableModel* m_model;
};
