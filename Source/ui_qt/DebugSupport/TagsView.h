#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>

namespace Ui
{
	class CTagsView;
}

#include "signal/Signal.h"
#include <functional>
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"

class CTagsView : public QMdiSubWindow
{
	Q_OBJECT

public:
	CTagsView(QMdiArea*);
	virtual ~CTagsView() = default;

	void Refresh();

	Framework::CSignal<void(uint32)> OnItemDblClick;
	Framework::CSignal<void(void)> OnStateChange;

public slots:
	void OnListDblClick(QTreeWidgetItem*, int);
	void OnNewClick();
	void OnRenameClick();
	void OnDeleteClick();

signals:
	void OnTagListChange();

protected:
	struct Strings
	{
		QString newTagString;
		QString renameTagString;
		QString deleteTagString;
		QString tagNameString;
		QString tagAddressString;
		QString deleteTagConfirmString;
		QString deleteModuleTagsConfirmString;
	};

	void SetStrings(const Strings&);

	void SetContext(CMIPS*, CMIPSTags*, CBiosDebugInfoProvider*);
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

	void RefreshList();
	void InitializeModuleGrouper();
	QTreeWidgetItem* GetTagGroup(uint32);

	Ui::CTagsView* ui;

	Framework::CSignal<void()>::Connection m_tagsChangeConnection;

	Strings m_strings;
	CMIPS* m_context = nullptr;
	CMIPSTags* m_tags = nullptr;
	BiosDebugModuleInfoArray m_modules;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider = nullptr;
	std::map<uint32, QTreeWidgetItem*> m_groupMap;
};
