#include "ELFSymbolView.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#include <QLabel>
#include <QFontMetrics>
#include <QHeaderView>

template <typename ElfType>
CELFSymbolView<ElfType>::CELFSymbolView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
    : QWidget(parent)
{
	QStringList colLabels = {"Name", "Address", "Size", "Type", "Binding", "Section"};

	m_layout = new QVBoxLayout(this);

	m_tableWidget = new QTableWidget(this);
	m_tableWidget->setRowCount(0);
	m_tableWidget->setColumnCount(6);
	m_tableWidget->setHorizontalHeaderLabels(colLabels);
	m_tableWidget->setSortingEnabled(true);

	m_tableWidget->verticalHeader()->hide();
	m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	m_layout->addWidget(m_tableWidget);

	groupBoxLayout->addWidget(this);
	hide();
}

template <typename ElfType>
void CELFSymbolView<ElfType>::Reset()
{
	m_tableWidget->setRowCount(0);
}

template <typename ElfType>
void CELFSymbolView<ElfType>::SetELF(ElfType* pELF)
{
	m_pELF = pELF;
	PopulateList();
}

template <typename ElfType>
void CELFSymbolView<ElfType>::PopulateList()
{
	const char* sectionName = ".symtab";

	auto pSymTab = m_pELF->FindSection(sectionName);
	if(pSymTab == NULL) return;

	const char* pStrTab = (const char*)m_pELF->GetSectionData(pSymTab->nIndex);
	if(pStrTab == NULL) return;

	auto symbols = reinterpret_cast<const typename ElfType::SYMBOL*>(m_pELF->FindSectionData(sectionName));
	unsigned int count = pSymTab->nSize / sizeof(typename ElfType::SYMBOL);

	m_tableWidget->setRowCount(count);

#define CASE_ELF_ENUM(enumValue) \
	case ELF::enumValue:         \
		sTemp = #enumValue;      \
		break;

	for(unsigned int i = 0; i < count; i++)
	{
		auto symbol = symbols[i];

		int j = 0;
		std::string sTemp;

		m_tableWidget->setItem(i, j++, new QTableWidgetItem(pStrTab + symbol.nName));
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(lexical_cast_hex<std::string>(symbol.nValue, 8).c_str()));
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(lexical_cast_hex<std::string>(symbol.nSize, 8).c_str()));

		switch(symbol.nInfo & 0x0F)
		{
			CASE_ELF_ENUM(STT_NOTYPE)
			CASE_ELF_ENUM(STT_OBJECT)
			CASE_ELF_ENUM(STT_FUNC)
			CASE_ELF_ENUM(STT_SECTION)
			CASE_ELF_ENUM(STT_FILE)
		default:
			sTemp = string_format("%i", symbol.nInfo & 0x0F);
			break;
		}
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(sTemp.c_str()));

		switch((symbol.nInfo >> 4) & 0xF)
		{
			CASE_ELF_ENUM(STB_LOCAL)
			CASE_ELF_ENUM(STB_GLOBAL)
			CASE_ELF_ENUM(STB_WEAK)
		default:
			sTemp = string_format("%i", (symbol.nInfo >> 4) & 0x0F);
			break;
		}
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(sTemp.c_str()));

		switch(symbol.nSectionIndex)
		{
			CASE_ELF_ENUM(SHN_UNDEF)
			CASE_ELF_ENUM(SHN_ABS)
		default:
			sTemp = string_format("%i", symbol.nSectionIndex);
			break;
		}
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(sTemp.c_str()));

#undef CASE_ELF_ENUM
	}
}

template class CELFSymbolView<CELF32>;
template class CELFSymbolView<CELF64>;
