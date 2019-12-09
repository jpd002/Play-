#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include "QtGenericTableModel.h"

#include "VirtualMachine.h"
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"
#include "Types.h"
#include "VirtualMachineStateView.h"

class CThreadsViewWnd : public QMdiSubWindow, public CVirtualMachineStateView
{
public:
	CThreadsViewWnd(QMdiArea*);
	virtual ~CThreadsViewWnd() = default;

	void HandleMachineStateChange() override;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);

	Framework::CSignal<void(uint32)> OnGotoAddress;

public slots:
	void tableDoubleClick(const QModelIndex&);

private:
	void Update();

	CMIPS* m_context;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider;
	QTableView* m_tableView;
	CQtGenericTableModel* m_model;
};
