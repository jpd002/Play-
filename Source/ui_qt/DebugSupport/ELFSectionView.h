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
#include "QtMemoryViewModel.h"

class CELFSectionView : public QWidget
{

public:
	CELFSectionView(QMdiSubWindow*, QLayout*);
	~CELFSectionView();

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
	void ShowContextMenu(const QPoint&);
	void ResizeColumns();
	void AutoColumn();
	void SetActiveUnit(int);
	void SetSelectionStart(uint32);

	CELF* m_pELF;
	QVBoxLayout* m_layout;
	std::vector<QLineEdit*> m_editFields;
	QTableView* m_memView;
	QTableWidget* m_dynSecTableWidget;
	CQtMemoryViewModel* m_model;
	uint8* m_data;

	int m_cwidth = 0;
	int m_bytesPerLine = 0;
	int m_maxUnits = 0;
};
