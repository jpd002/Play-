#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>

#include "signal/Signal.h"
#include <functional>
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"
#include "ELF.h"

class CFunctionsView : public QMdiSubWindow
{
	Q_OBJECT

public:
	CFunctionsView(QMdiArea*);
	virtual ~CFunctionsView() = default;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);
	void Refresh();

	Framework::CSignal<void(uint32)> OnFunctionDblClick;
	Framework::CSignal<void(void)> OnFunctionsStateChange;

public slots:
	void OnListDblClick(QTreeWidgetItem*, int);
	void OnNewClick();
	void OnRenameClick();
	void OnDeleteClick();
	void OnImportClick();

signals:
	void OnTagListChange();

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

private:
	void RefreshList();
	void InitializeModuleGrouper();
	QTreeWidgetItem* GetFunctionGroup(uint32);

	Framework::CSignal<void()>::Connection m_functionTagsChangeConnection;

	CMIPS* m_context = nullptr;
	BiosDebugModuleInfoArray m_modules;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider = nullptr;
	QTreeWidget* m_treeWidget;
	std::map<uint32, QTreeWidgetItem*> m_groupMap;
};
