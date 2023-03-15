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

template <typename ElfType>
class CELFView : public QMdiSubWindow
{
public:
	CELFView(QMdiArea*);
	~CELFView() = default;

	void SetELF(ElfType*);
	void Reset();

protected:
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

private:
	void PopulateList();
	void itemSelectionChanged();

	ElfType* m_pELF = nullptr;
	QWidget* m_centralwidget;
	QHBoxLayout* m_layout;
	QTreeWidget* m_treeWidget;
	QGroupBox* m_groupBox;

	CELFHeaderView<ElfType>* m_pHeaderView;
	CELFSymbolView<ElfType>* m_pSymbolView;
	CELFSectionView<ElfType>* m_pSectionView;
	CELFProgramView<ElfType>* m_pProgramView;

	std::vector<QLineEdit*> m_editFields;
	bool m_hasPrograms = false;
	bool m_hasSymbols = false;
};
