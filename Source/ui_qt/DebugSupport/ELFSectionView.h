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

template <typename ElfType>
class CELFSectionView : public QWidget
{
public:
	CELFSectionView(QMdiSubWindow*, QLayout*);
	~CELFSectionView() = default;

	void SetELF(ElfType*);
	void SetSection(int);
	void SetBytesPerLine(int);
	void ResizeEvent();
	void Reset();

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

private:
	void FillInformation(int);
	void FillDynamicSectionListView(int);

	ElfType* m_pELF = nullptr;
	QVBoxLayout* m_layout;
	std::vector<QLineEdit*> m_editFields;
	CMemoryViewTable* m_memView;
	QTableWidget* m_dynSecTableWidget;
	uint8* m_data = nullptr;
};
