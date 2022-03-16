#include "ELFHeaderView.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#include <QLabel>
#include <QFontMetrics>

CELFHeaderView::CELFHeaderView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
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

void CELFHeaderView::Reset()
{
	for(auto editField : m_editFields)
	{
		editField->clear();
	}
}

void CELFHeaderView::SetELF(CELF* pELF)
{
	m_pELF = pELF;
	FillInformation();
}

void CELFHeaderView::FillInformation()
{
	int i = 0;
	std::string sTemp;
	auto pH = &m_pELF->GetHeader();

	switch(pH->nType)
	{
	case CELF::ET_NONE:
		sTemp = ("ET_NONE");
		break;
	case CELF::ET_REL:
		sTemp = ("ET_REL");
		break;
	case CELF::ET_EXEC:
		sTemp = ("ET_EXEC");
		break;
	case CELF::ET_DYN:
		sTemp = ("ET_DYN");
		break;
	case CELF::ET_CORE:
		sTemp = ("ET_CORE");
		break;
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nType);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

	switch(pH->nCPU)
	{
	case CELF::EM_NONE:
		sTemp = ("EM_NONE");
		break;
	case CELF::EM_M32:
		sTemp = ("EM_M32");
		break;
	case CELF::EM_SPARC:
		sTemp = ("EM_SPARC");
		break;
	case CELF::EM_386:
		sTemp = ("EM_386");
		break;
	case CELF::EM_68K:
		sTemp = ("EM_68K");
		break;
	case CELF::EM_88K:
		sTemp = ("EM_88K");
		break;
	case CELF::EM_860:
		sTemp = ("EM_860");
		break;
	case CELF::EM_MIPS:
		sTemp = ("EM_MIPS");
		break;
	case CELF::EM_PPC64:
		sTemp = ("EM_PPC64");
		break;
	case CELF::EM_ARM:
		sTemp = ("EM_ARM");
		break;
	default:
		sTemp = string_format(("Unknown (%i)"), pH->nCPU);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

	switch(pH->nVersion)
	{
	case CELF::EV_NONE:
		sTemp = ("EV_NONE");
		break;
	case CELF::EV_CURRENT:
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
