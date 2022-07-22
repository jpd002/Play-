#pragma once

#include <QMdiSubWindow>
#include <QWidget>
#include <QTreeWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QLineEdit>

#include "ELF.h"

template <typename ElfType>
class CELFHeaderView : public QWidget
{
public:
	CELFHeaderView(QMdiSubWindow*, QLayout*);
	~CELFHeaderView() = default;

	void SetELF(ElfType*);
	void Reset();

private:
	void FillInformation();

	ElfType* m_pELF = nullptr;
	QVBoxLayout* m_layout;
	std::vector<QLineEdit*> m_editFields;
};
