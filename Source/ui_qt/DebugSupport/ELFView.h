#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>

#include "ELF.h"
#include "ELFHeaderView.h"
#include "ELFSymbolView.h"
#include "ELFSectionView.h"
#include "ELFProgramView.h"

class CELFView : public QMdiSubWindow
{
public:
	CELFView(QMdiArea*);
	~CELFView() = default;

	void SetELF(CELF*);
	void Reset();

protected:
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

private:
	void PopulateList();
	void itemSelectionChanged();

	CELF* m_pELF = nullptr;
	QWidget* m_centralwidget;
	QHBoxLayout* m_layout;
	QTreeWidget* m_treeWidget;
	QGroupBox* m_groupBox;

	CELFHeaderView* m_pHeaderView;
	CELFSymbolView* m_pSymbolView;
	CELFSectionView* m_pSectionView;
	CELFProgramView* m_pProgramView;

	std::vector<QLineEdit*> m_editFields;
	bool m_hasPrograms = false;
	bool m_hasSymbols = false;
};
