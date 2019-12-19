#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include "QtGenericTableModel.h"

#include "signal/Signal.h"
#include <functional>
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"
#include "ELF.h"

class CFunctionsView : public QMdiSubWindow
{
public:
	CFunctionsView(QMdiArea*);
	virtual ~CFunctionsView() = default;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);
	void Refresh();

	Framework::CSignal<void(uint32)> OnFunctionDblClick;
	Framework::CSignal<void(void)> OnFunctionsStateChange;

public slots:
	void OnListDblClick(const QModelIndex&);
	void OnNewClick();
	void OnRenameClick();
	void OnDeleteClick();
	void OnImportClick();

private:
	void RefreshList();
	void InitializeModuleGrouper();
	uint32 GetFunctionGroupId(uint32);

	Framework::CSignal<void()>::Connection m_functionTagsChangeConnection;

	CMIPS* m_context = nullptr;
	BiosDebugModuleInfoArray m_modules;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider = nullptr;
	QTableView* m_tableView;
	CQtGenericTableModel* m_model;
};
