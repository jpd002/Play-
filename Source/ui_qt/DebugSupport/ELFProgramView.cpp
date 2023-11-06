#include "ELFProgramView.h"
#include "string_format.h"

#include <QLabel>
#include <QFontMetrics>

template <typename ElfType>
CELFProgramView<ElfType>::CELFProgramView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
    : QWidget(parent)
{

	std::vector<std::string> labelsStr = {"Type:", "Offset:", "Virtual Address:", "Physical Address:", "File Size:", "Memory Size:", "Flags:", "Alignment:"};
	m_layout = new QVBoxLayout(this);

	auto label = new QLabel(this);
	QFontMetrics metric(label->font());
	delete label;
	auto labelWidth = metric.horizontalAdvance(labelsStr.back().c_str()) + 10;

	for(auto labelStr : labelsStr)
	{
		auto horizontalLayout = new QHBoxLayout();
		auto label = new QLabel(this);
		label->setText(labelStr.c_str());
		label->setFixedWidth(labelWidth);

		horizontalLayout->addWidget(label);

		auto lineEdit = new QLineEdit(this);
		lineEdit->setReadOnly(true);
		m_editFields.push_back(lineEdit);

		horizontalLayout->addWidget(lineEdit);
		m_layout->addLayout(horizontalLayout);
	}

	// blankLayout / blankWidget funtion is to exapnd and take the empty space
	auto blankWidget = new QWidget(this);
	auto blankLayout = new QVBoxLayout();
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	blankWidget->setSizePolicy(sizePolicy);
	blankLayout->addWidget(blankWidget);
	m_layout->addLayout(blankLayout);

	groupBoxLayout->addWidget(this);
	hide();
}

template <typename ElfType>
void CELFProgramView<ElfType>::Reset()
{
	for(auto editField : m_editFields)
	{
		editField->clear();
	}
}

template <typename ElfType>
void CELFProgramView<ElfType>::SetProgram(int program)
{
	FillInformation(program);
}

template <typename ElfType>
void CELFProgramView<ElfType>::SetELF(ElfType* pELF)
{
	m_pELF = pELF;
}

template <typename ElfType>
void CELFProgramView<ElfType>::FillInformation(int program)
{
	int i = 0;
	std::string sTemp;
	auto pH = m_pELF->GetProgram(program);

#define CASE_ELF_ENUM(enumValue) \
	case ELF::enumValue:         \
		sTemp = #enumValue;      \
		break;

	switch(pH->nType)
	{
		CASE_ELF_ENUM(PT_NULL)
		CASE_ELF_ENUM(PT_LOAD)
		CASE_ELF_ENUM(PT_DYNAMIC)
		CASE_ELF_ENUM(PT_INTERP)
		CASE_ELF_ENUM(PT_NOTE)
		CASE_ELF_ENUM(PT_SHLIB)
		CASE_ELF_ENUM(PT_PHDR)
		CASE_ELF_ENUM(PT_TLS)
	default:
		sTemp = string_format("Unknown (0x%08X)", pH->nType);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

#undef CASE_ELF_ENUM

	sTemp = string_format("0x%08X", pH->nOffset);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nVAddress);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nPAddress);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nFileSize);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nMemorySize);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nFlags);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nAlignment);
	m_editFields[i++]->setText(sTemp.c_str());
}

template class CELFProgramView<CELF32>;
template class CELFProgramView<CELF64>;
