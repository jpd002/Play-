#include "ELFSymbolView.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#include <QLabel>
#include <QFontMetrics>
#include <QHeaderView>

CELFSymbolView::CELFSymbolView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
    : QWidget(parent)
{

	QStringList colLabels = {"Name", "Address", "Size", "Type", "Binding", "Section"};

	m_layout = new QVBoxLayout(this);

	m_tableWidget = new QTableWidget(this);
	m_tableWidget->setRowCount(0);
	m_tableWidget->setColumnCount(6);
	m_tableWidget->setHorizontalHeaderLabels(colLabels);

	m_tableWidget->verticalHeader()->hide();
	m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	m_layout->addWidget(m_tableWidget);

	groupBoxLayout->addWidget(this);
	hide();
}

void CELFSymbolView::Reset()
{
	m_tableWidget->setRowCount(0);
}

void CELFSymbolView::SetELF(CELF* pELF)
{
	m_pELF = pELF;
	PopulateList();
}

void CELFSymbolView::PopulateList()
{
	// m_sortState = SORT_STATE_NONE;
	const char* sectionName = ".symtab";

	auto pSymTab = m_pELF->FindSection(sectionName);
	if(pSymTab == NULL) return;

	const char* pStrTab = (const char*)m_pELF->GetSectionData(pSymTab->nIndex);
	if(pStrTab == NULL) return;

	auto symbols = reinterpret_cast<const CELF::ELFSYMBOL*>(m_pELF->FindSectionData(sectionName));
	unsigned int count = pSymTab->nSize / sizeof(CELF::ELFSYMBOL);

	m_tableWidget->setRowCount(count);

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
		case 0x00:
			sTemp = "STT_NOTYPE";
			break;
		case 0x01:
			sTemp = "STT_OBJECT";
			break;
		case 0x02:
			sTemp = "STT_FUNC";
			break;
		case 0x03:
			sTemp = "STT_SECTION";
			break;
		case 0x04:
			sTemp = "STT_FILE";
			break;
		default:
			sTemp = string_format("%i", symbol.nInfo & 0x0F);
			break;
		}
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(sTemp.c_str()));

		switch((symbol.nInfo >> 4) & 0xF)
		{
		case 0x00:
			sTemp = "STB_LOCAL";
			break;
		case 0x01:
			sTemp = "STB_GLOBAL";
			break;
		case 0x02:
			sTemp = "STB_WEAK";
			break;
		default:
			sTemp = string_format("%i", (symbol.nInfo >> 4) & 0x0F);
			break;
		}
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(sTemp.c_str()));

		switch(symbol.nSectionIndex)
		{
		case 0:
			sTemp = "SHN_UNDEF";
			break;
		case 0xFFF1:
			sTemp = "SHN_ABS";
			break;
		default:
			sTemp = string_format("%i", symbol.nSectionIndex);
			break;
		}
		m_tableWidget->setItem(i, j++, new QTableWidgetItem(sTemp.c_str()));
	}
}
