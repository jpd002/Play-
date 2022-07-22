#pragma once

#include <QMdiSubWindow>
#include <QWidget>
#include <QTableWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QLineEdit>

#include "ELF.h"

template <typename ElfType>
class CELFSymbolView : public QWidget
{
public:
	CELFSymbolView(QMdiSubWindow*, QLayout*);
	~CELFSymbolView() = default;

	void SetELF(ElfType*);
	void Reset();

private:
	void PopulateList();

	ElfType* m_pELF = nullptr;
	QVBoxLayout* m_layout;
	QTableWidget* m_tableWidget;
};
