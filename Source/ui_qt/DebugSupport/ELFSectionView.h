#pragma once

#include <QGroupBox>
#include <QGridLayout>
#include <QTableWidget>
#include <QTableView>
#include <QTreeWidget>
#include <QLineEdit>
#include <QMdiSubWindow>
#include <QWidget>

#include "ELF.h"
#include "MemoryViewTable.h"

class CELFSectionView : public QWidget
{

public:
	CELFSectionView(QMdiSubWindow*, QLayout*);
	~CELFSectionView() = default;

	void SetELF(CELF*);
	void SetSection(int);
	void SetBytesPerLine(int);
	void ResizeEvent();
	void Reset();

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

private:
	void FillInformation(int);
	void FillDynamicSectionListView(int);

	CELF* m_pELF = nullptr;
	QVBoxLayout* m_layout;
	std::vector<QLineEdit*> m_editFields;
	CMemoryViewTable* m_memView;
	QTableWidget* m_dynSecTableWidget;
	uint8* m_data = nullptr;
};
