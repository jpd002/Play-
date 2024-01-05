#include "ELFHeaderView.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#include <QLabel>
#include <QFontMetrics>

template <typename ElfType>
CELFHeaderView<ElfType>::CELFHeaderView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
    : QWidget(parent)
{
	std::vector<std::string> labelsStr = {"Type:", "Machine:", "Version:", "Entry Point:", "Flags:", "Header Size:", "Program Header Table Offset:", "Program Header Size:", "Program Header Count:", "Section Header Table Offset:", "Section Header Size:", "Section Header Count:", "Section Header String Table Index:"};
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

	m_layout->addStretch();

	groupBoxLayout->addWidget(this);
	hide();
}

template <typename ElfType>
void CELFHeaderView<ElfType>::Reset()
{
	for(auto editField : m_editFields)
	{
		editField->clear();
	}
}

template <typename ElfType>
void CELFHeaderView<ElfType>::SetELF(ElfType* pELF)
{
	m_pELF = pELF;
	FillInformation();
}

template <typename ElfType>
void CELFHeaderView<ElfType>::FillInformation()
{
	int i = 0;
	std::string sTemp;
	const auto* pH = &m_pELF->GetHeader();

#define CASE_ELF_ENUM(enumValue) \
	case ELF::enumValue:         \
		sTemp = #enumValue;      \
		break;

	switch(pH->nType)
	{
		CASE_ELF_ENUM(ET_NONE)
		CASE_ELF_ENUM(ET_REL)
		CASE_ELF_ENUM(ET_EXEC)
		CASE_ELF_ENUM(ET_DYN)
		CASE_ELF_ENUM(ET_CORE)
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nType);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

	switch(pH->nCPU)
	{
		CASE_ELF_ENUM(EM_NONE)
		CASE_ELF_ENUM(EM_M32)
		CASE_ELF_ENUM(EM_SPARC)
		CASE_ELF_ENUM(EM_386)
		CASE_ELF_ENUM(EM_68K)
		CASE_ELF_ENUM(EM_88K)
		CASE_ELF_ENUM(EM_860)
		CASE_ELF_ENUM(EM_MIPS)
		CASE_ELF_ENUM(EM_PPC64)
		CASE_ELF_ENUM(EM_SPU)
		CASE_ELF_ENUM(EM_ARM)
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nCPU);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

	switch(pH->nVersion)
	{
		CASE_ELF_ENUM(EV_NONE)
		CASE_ELF_ENUM(EV_CURRENT)
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nVersion);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

#undef CASE_ELF_ENUM

	m_editFields[i++]->setText((("0x") + lexical_cast_hex<std::string>(pH->nEntryPoint, 8)).c_str());
	m_editFields[i++]->setText((("0x") + lexical_cast_hex<std::string>(pH->nFlags, 8)).c_str());
	m_editFields[i++]->setText((("0x") + lexical_cast_hex<std::string>(pH->nSize, 8)).c_str());
	m_editFields[i++]->setText((("0x") + lexical_cast_hex<std::string>(pH->nProgHeaderStart, 8)).c_str());
	m_editFields[i++]->setText((("0x") + lexical_cast_hex<std::string>(pH->nProgHeaderEntrySize, 8)).c_str());
	m_editFields[i++]->setText(lexical_cast_uint<std::string>(pH->nProgHeaderCount).c_str());
	m_editFields[i++]->setText((("0x") + lexical_cast_hex<std::string>(pH->nSectHeaderStart, 8)).c_str());
	m_editFields[i++]->setText((("0x") + lexical_cast_hex<std::string>(pH->nSectHeaderEntrySize, 8)).c_str());
	m_editFields[i++]->setText(lexical_cast_uint<std::string>(pH->nSectHeaderCount).c_str());
	m_editFields[i++]->setText(lexical_cast_uint<std::string>(pH->nSectHeaderStringTableIndex).c_str());
}

template class CELFHeaderView<CELF32>;
template class CELFHeaderView<CELF64>;
