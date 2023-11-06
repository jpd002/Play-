#include "ELFSectionView.h"
#include "string_format.h"
#include "StringUtils.h"
#include "lexical_cast_ex.h"

#include <QLabel>
#include <QAction>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QFontDatabase>
#include <QFontMetrics>

template <typename ElfType>
CELFSectionView<ElfType>::CELFSectionView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
    : QWidget(parent)
{

	std::vector<std::string> labelsStr = {"Type:", "Flags:", "Address:", "File Offset:", "Size:", "Section Link:", "Info:", "Alignment:", "Entry Size:"};
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

	m_memView = new CMemoryViewTable(this);
	auto verticalLayout = new QHBoxLayout();
	verticalLayout->addWidget(m_memView);

	m_dynSecTableWidget = new QTableWidget(this);
	m_dynSecTableWidget->setRowCount(0);
	m_dynSecTableWidget->setColumnCount(2);
	m_dynSecTableWidget->setHorizontalHeaderLabels({"Type", "Value"});
	m_dynSecTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	verticalLayout->addWidget(m_dynSecTableWidget);

	m_layout->addLayout(verticalLayout);

	groupBoxLayout->addWidget(this);
	hide();
}

template <typename ElfType>
void CELFSectionView<ElfType>::SetSection(int section)
{
	FillInformation(section);
}

template <typename ElfType>
void CELFSectionView<ElfType>::Reset()
{
	for(auto editField : m_editFields)
	{
		editField->clear();
	}
}

template <typename ElfType>
void CELFSectionView<ElfType>::showEvent(QShowEvent* evt)
{
	QWidget::showEvent(evt);
	m_memView->ShowEvent();
}

template <typename ElfType>
void CELFSectionView<ElfType>::ResizeEvent()
{
	m_memView->ResizeEvent();
}

template <typename ElfType>
void CELFSectionView<ElfType>::SetBytesPerLine(int bytesForLine)
{
	m_memView->SetBytesPerLine(bytesForLine);
}

template <typename ElfType>
void CELFSectionView<ElfType>::SetELF(ElfType* pELF)
{
	m_pELF = pELF;
}

template <typename ElfType>
void CELFSectionView<ElfType>::FillInformation(int section)
{
	int i = 0;
	std::string sTemp;
	auto pH = m_pELF->GetSection(section);

#define CASE_ELF_ENUM(enumValue) \
	case ELF::enumValue:         \
		sTemp = #enumValue;      \
		break;

	switch(pH->nType)
	{
		CASE_ELF_ENUM(SHT_NULL)
		CASE_ELF_ENUM(SHT_PROGBITS)
		CASE_ELF_ENUM(SHT_SYMTAB)
		CASE_ELF_ENUM(SHT_STRTAB)
		CASE_ELF_ENUM(SHT_RELA)
		CASE_ELF_ENUM(SHT_HASH)
		CASE_ELF_ENUM(SHT_DYNAMIC)
		CASE_ELF_ENUM(SHT_NOTE)
		CASE_ELF_ENUM(SHT_NOBITS)
		CASE_ELF_ENUM(SHT_REL)
		CASE_ELF_ENUM(SHT_DYNSYM)
		CASE_ELF_ENUM(SHT_SYMTAB_SHNDX)
	default:
		sTemp = string_format("Unknown (0x%08X)", pH->nType);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

#undef CASE_ELF_ENUM

	std::vector<std::string> flagNames;

#define ADD_ELF_FLAG(flagValue)     \
	if(pH->nFlags & ELF::flagValue) \
		flagNames.push_back(#flagValue);

	ADD_ELF_FLAG(SHF_WRITE)
	ADD_ELF_FLAG(SHF_ALLOC)
	ADD_ELF_FLAG(SHF_EXECINSTR)
	ADD_ELF_FLAG(SHF_TLS)

#undef ADD_ELF_FLAG

	sTemp = string_format("0x%08X", pH->nFlags);

	if(!flagNames.empty())
	{
		sTemp += string_format(" (%s)", StringUtils::Join(flagNames, " | ").c_str());
	}

	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nStart);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nOffset);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nSize);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("%i", pH->nIndex);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("%i", pH->nInfo);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nAlignment);
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nOther);
	m_editFields[i++]->setText(sTemp.c_str());

	if(pH->nType == ELF::SHT_DYNAMIC)
	{
		FillDynamicSectionListView(section);
		m_dynSecTableWidget->show();
		m_memView->SetData(nullptr, 0);
		m_memView->hide();
	}
	else if(pH->nType != ELF::SHT_NOBITS)
	{
		uint8* data = (uint8*)m_pELF->GetSectionData(section);
		auto getByte = [data](uint32 offset) {
			return data[offset];
		};
		m_memView->SetData(getByte, pH->nSize);
		m_memView->show();
		m_dynSecTableWidget->hide();
	}
	else
	{
		m_memView->SetData(nullptr, 0);
		m_memView->show();
		m_dynSecTableWidget->hide();
	}
}

template <typename ElfType>
void CELFSectionView<ElfType>::FillDynamicSectionListView(int section)
{
	m_dynSecTableWidget->setRowCount(0);

	auto pH = m_pELF->GetSection(section);
	const uint32* dynamicData = reinterpret_cast<const uint32*>(m_pELF->GetSectionData(section));
	const char* stringTable = (pH->nOther != -1) ? reinterpret_cast<const char*>(m_pELF->GetSectionData(pH->nIndex)) : NULL;

	for(unsigned int i = 0; i < pH->nSize; i += 8, dynamicData += 2)
	{
		uint32 tag = *(dynamicData + 0);
		uint32 value = *(dynamicData + 1);

		if(tag == 0) break;

		std::string tempTag;
		std::string tempVal;

		switch(tag)
		{
		case ELF::DT_NEEDED:
			tempTag = "DT_NEEDED";
			tempVal = stringTable + value;
			break;
		case ELF::DT_PLTRELSZ:
			tempTag = "DT_PLTRELSZ";
			tempVal = string_format("0x%08X", value);
			break;
		case ELF::DT_PLTGOT:
			tempTag = "DT_PLTGOT";
			tempVal = string_format("0x%08X", value);
			break;
		case ELF::DT_HASH:
			tempTag = "DT_HASH";
			tempVal = string_format("0x%08X", value);
			break;
		case ELF::DT_STRTAB:
			tempTag = "DT_STRTAB";
			tempVal = string_format("0x%08X", value);
			break;
		case ELF::DT_SYMTAB:
			tempTag = "DT_SYMTAB";
			tempVal = string_format("0x%08X", value);
			break;
		case ELF::DT_SONAME:
			tempTag = "DT_SONAME";
			tempVal = stringTable + value;
			break;
		case ELF::DT_SYMBOLIC:
			tempTag = "DT_SYMBOLIC";
			tempVal = "";
			break;
		default:
			tempTag = string_format("Unknown (0x%08X)", tag);
			tempVal = string_format("0x%08X", value);
			break;
		}

		m_dynSecTableWidget->setRowCount((i / 8) + 1);
		m_dynSecTableWidget->setItem((i / 8), 0, new QTableWidgetItem(tempTag.c_str()));
		m_dynSecTableWidget->setItem((i / 8), 1, new QTableWidgetItem(tempVal.c_str()));
	}
}

template class CELFSectionView<CELF32>;
template class CELFSectionView<CELF64>;
