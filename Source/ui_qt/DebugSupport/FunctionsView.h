#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include "QtGenericTableModel.h"

#include "signal/Signal.h"
#include <functional>
// #include "win32/MDIChild.h"
// #include "win32/ListView.h"
// #include "win32/Button.h"
// #include "layout/VerticalLayout.h"
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

private:
	void RefreshList();
	void InitializeModuleGrouper();
	uint32 GetFunctionGroupId(uint32);

	// void OnListDblClick();
	// void OnNewClick();
	// void OnRenameClick();
	// void OnDeleteClick();
	// void OnImportClick();

	// Framework::Win32::CListView* m_pList = nullptr;
	// Framework::Win32::CButton* m_pNew = nullptr;
	// Framework::Win32::CButton* m_pRename = nullptr;
	// Framework::Win32::CButton* m_pDelete = nullptr;
	// Framework::Win32::CButton* m_pImport = nullptr;

	// Framework::FlatLayoutPtr m_pLayout;

	Framework::CSignal<void()>::Connection m_functionTagsChangeConnection;

	CMIPS* m_context = nullptr;
	BiosDebugModuleInfoArray m_modules;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider = nullptr;
	QTableView* m_tableView;
	CQtGenericTableModel* m_model;
};
