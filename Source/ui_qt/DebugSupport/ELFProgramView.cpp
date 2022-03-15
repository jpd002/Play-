#include "ELFProgramView.h"
#include "string_format.h"

#include <QLabel>
#include <QFontMetrics>

CELFProgramView::CELFProgramView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
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

void CELFProgramView::Reset()
{
	for(auto editField : m_editFields)
	{
		editField->clear();
	}
}

void CELFProgramView::SetProgram(int program)
{
	FillInformation(program);
}

void CELFProgramView::SetELF(CELF* pELF)
{
	m_pELF = pELF;
}

void CELFProgramView::FillInformation(int program)
{
	int i = 0;
	std::string sTemp;
	auto pH = m_pELF->GetProgram(program);

	switch(pH->nType)
	{
	case CELF::PT_NULL:
		sTemp = "PT_NULL";
		break;
	case CELF::PT_LOAD:
		sTemp = "PT_LOAD";
		break;
	case CELF::PT_DYNAMIC:
		sTemp = "PT_DYNAMIC";
		break;
	case CELF::PT_INTERP:
		sTemp = "PT_INTERP";
		break;
	case CELF::PT_NOTE:
		sTemp = "PT_NOTE";
		break;
	case CELF::PT_SHLIB:
		sTemp = "PT_SHLIB";
		break;
	case CELF::PT_PHDR:
		sTemp = "PT_PHDR";
		break;
	default:
		sTemp = string_format("Unknown (0x%08X)", pH->nType);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

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
