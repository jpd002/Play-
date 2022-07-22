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
	auto pH = &m_pELF->GetHeader();

	switch(pH->nType)
	{
	case ELF::ET_NONE:
		sTemp = ("ET_NONE");
		break;
	case ELF::ET_REL:
		sTemp = ("ET_REL");
		break;
	case ELF::ET_EXEC:
		sTemp = ("ET_EXEC");
		break;
	case ELF::ET_DYN:
		sTemp = ("ET_DYN");
		break;
	case ELF::ET_CORE:
		sTemp = ("ET_CORE");
		break;
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nType);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

	switch(pH->nCPU)
	{
	case ELF::EM_NONE:
		sTemp = ("EM_NONE");
		break;
	case ELF::EM_M32:
		sTemp = ("EM_M32");
		break;
	case ELF::EM_SPARC:
		sTemp = ("EM_SPARC");
		break;
	case ELF::EM_386:
		sTemp = ("EM_386");
		break;
	case ELF::EM_68K:
		sTemp = ("EM_68K");
		break;
	case ELF::EM_88K:
		sTemp = ("EM_88K");
		break;
	case ELF::EM_860:
		sTemp = ("EM_860");
		break;
	case ELF::EM_MIPS:
		sTemp = ("EM_MIPS");
		break;
	case ELF::EM_PPC64:
		sTemp = ("EM_PPC64");
		break;
	case ELF::EM_ARM:
		sTemp = ("EM_ARM");
		break;
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nCPU);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

	switch(pH->nVersion)
	{
	case ELF::EV_NONE:
		sTemp = ("EV_NONE");
		break;
	case ELF::EV_CURRENT:
		sTemp = ("EV_CURRENT");
		break;
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nVersion);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

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
