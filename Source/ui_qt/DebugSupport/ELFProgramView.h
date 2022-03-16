#pragma once

#include <QMdiSubWindow>
#include <QWidget>
#include <QTreeWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QLineEdit>

#include "ELF.h"

class CELFProgramView : public QWidget
{

public:
	CELFProgramView(QMdiSubWindow*, QLayout*);
	~CELFProgramView() = default;

	void SetELF(CELF*);
	void SetProgram(int);
	void Reset();

private:
	void FillInformation(int);

	CELF* m_pELF = nullptr;
	QVBoxLayout* m_layout;
	std::vector<QLineEdit*> m_editFields;
};
