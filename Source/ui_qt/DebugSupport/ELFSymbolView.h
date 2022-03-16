#pragma once

#include <QMdiSubWindow>
#include <QWidget>
#include <QTableWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QLineEdit>

#include "ELF.h"

class CELFSymbolView : public QWidget
{

public:
	CELFSymbolView(QMdiSubWindow*, QLayout*);
	~CELFSymbolView() = default;

	void SetELF(CELF*);
	void Reset();

private:
	void PopulateList();

	CELF* m_pELF = nullptr;
	QVBoxLayout* m_layout;
	QTableWidget* m_tableWidget;
};
