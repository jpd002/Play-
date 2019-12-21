#include "ELFSectionView.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#include <QLabel>
#include <QAction>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QFontDatabase>
#include <QFontMetrics>


CELFSectionView::CELFSectionView(QMdiSubWindow* parent, QLayout* groupBoxLayout)
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
		auto horizontalLayout = new QHBoxLayout(this);
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


	m_model = new CQtMemoryViewModel(this);
	m_memView = new QTableView(this);
	m_memView->setModel(m_model);

	QFont font = m_memView->font();
	QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	fixedFont.setPointSize(font.pointSize());
	m_memView->setFont(fixedFont);

	QFontMetrics fmetric(fixedFont);
	m_cwidth = fmetric.maxWidth();

	auto header = m_memView->horizontalHeader();
	header->setMinimumSectionSize(m_cwidth);
	header->hide();
	ResizeColumns();

	m_memView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_memView, &QTableView::customContextMenuRequested, this, &CELFSectionView::ShowContextMenu);

	auto verticalLayout = new QHBoxLayout(this);
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

CELFSectionView::~CELFSectionView()
{
}

void CELFSectionView::SetSection(int section)
{
	FillInformation(section);
}

void CELFSectionView::Reset()
{
	for(auto editField : m_editFields)
	{
		editField->clear();
	}
}

void CELFSectionView::showEvent(QShowEvent* evt)
{
	QWidget::showEvent(evt);
	AutoColumn();
}

void CELFSectionView::ResizeEvent()
{
	if(!m_model || m_bytesPerLine)
		return;

	AutoColumn();
}

void CELFSectionView::ShowContextMenu(const QPoint& pos)
{
	auto rightClickMenu = new QMenu(this);

	{
		auto bytesPerLineMenu = rightClickMenu->addMenu(tr("&Bytes Per Line"));
		auto bytesPerLineActionGroup = new QActionGroup(this);
		bytesPerLineActionGroup->setExclusive(true);

		auto autoAction = bytesPerLineMenu->addAction("auto");
		autoAction->setChecked(m_bytesPerLine == 0);
		autoAction->setCheckable(true);
		bytesPerLineActionGroup->addAction(autoAction);
		connect(autoAction, &QAction::triggered, std::bind(&CELFSectionView::SetBytesPerLine, this, 0));

		auto byte16Action = bytesPerLineMenu->addAction("16 bytes");
		byte16Action->setChecked(m_bytesPerLine == 16);
		byte16Action->setCheckable(true);
		bytesPerLineActionGroup->addAction(byte16Action);
		connect(byte16Action, &QAction::triggered, std::bind(&CELFSectionView::SetBytesPerLine, this, 16));

		auto byte32Action = bytesPerLineMenu->addAction("32 bytes");
		byte32Action->setChecked(m_bytesPerLine == 32);
		byte32Action->setCheckable(true);
		bytesPerLineActionGroup->addAction(byte32Action);
		connect(byte32Action, &QAction::triggered, std::bind(&CELFSectionView::SetBytesPerLine, this, 32));
	}

	{
		auto unitMenu = rightClickMenu->addMenu(tr("&Display Unit"));
		auto unitActionGroup = new QActionGroup(this);
		unitActionGroup->setExclusive(true);

		auto activeUnit = m_model->GetActiveUnit();
		for(uint32 i = 0; i < CQtMemoryViewModel::g_units.size(); i++)
		{
			const auto& unit = CQtMemoryViewModel::g_units[i];
			auto itemAction = unitMenu->addAction(unit.description);
			itemAction->setChecked(i == activeUnit);
			itemAction->setCheckable(true);
			unitActionGroup->addAction(itemAction);
			connect(itemAction, &QAction::triggered, std::bind(&CELFSectionView::SetActiveUnit, this, i));
		}
	}

	rightClickMenu->popup(m_memView->viewport()->mapToGlobal(pos));
}

void CELFSectionView::ResizeColumns()
{
	auto header = m_memView->horizontalHeader();
	header->setSectionResizeMode(QHeaderView::Fixed);

	auto units = m_model->UnitsForCurrentLine();
	auto valueCell = m_cwidth * (m_model->CharsPerUnit() + 2);
	int asciiCell = m_cwidth * (m_model->GetBytesPerUnit());
	for(auto i = 0; i < units; ++i)
	{
		header->resizeSection(i, valueCell);
	}
	header->resizeSection(units, (units * asciiCell) + (m_cwidth * 2));

	// collapse unused column
	for(auto i = units + 1; i < m_maxUnits; ++i)
	{
		header->resizeSection(i, 0);
	}
	m_maxUnits = units + 1;
}

void CELFSectionView::AutoColumn()
{
	QFont font = m_memView->font();
	QFontMetrics metric(font);
	int columnHeaderWidth = metric.horizontalAdvance(" 0x00000000");

	int tableWidth = m_memView->size().width() - columnHeaderWidth - m_memView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	auto bytesPerUnit = m_model->GetBytesPerUnit();

	int valueCell = m_cwidth * (m_model->CharsPerUnit() + 2);
	int asciiCell = m_cwidth * (m_model->GetBytesPerUnit());

	int i = 0x2;
	while(true)
	{
		int valueCellsWidth = i * valueCell;
		int asciiCellsWidth = (i * asciiCell) + (m_cwidth * 2);
		int totalWidth = valueCellsWidth + asciiCellsWidth;
		if(totalWidth > tableWidth)
		{
			--i;
			break;
		}
		++i;
	}
	m_model->SetUnitsForCurrentLine(i);
	ResizeColumns();
	m_model->Redraw();
}

void CELFSectionView::SetActiveUnit(int index)
{
	m_model->SetActiveUnit(index);
	AutoColumn();
}

void CELFSectionView::SetSelectionStart(uint32 address)
{
	auto column = address % m_model->BytesForCurrentLine();
	auto row = (address - column) / m_model->UnitsForCurrentLine();

	auto index = m_model->index(row, column);
	m_memView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
	m_memView->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void CELFSectionView::SetBytesPerLine(int bytesForLine)
{
	m_bytesPerLine = bytesForLine;
	if(bytesForLine)
	{
		m_model->SetUnitsForCurrentLine(bytesForLine);
		ResizeColumns();
		m_model->Redraw();
	}
	else
	{
		AutoColumn();
	}
}

void CELFSectionView::SetELF(CELF* pELF)
{
	m_pELF = pELF;
}


void CELFSectionView::FillInformation(int section)
{
	int i = 0;
	std::string sTemp;
	ELFSECTIONHEADER* pH = m_pELF->GetSection(section);

	switch(pH->nType)
	{
	case CELF::SHT_NULL:
		sTemp = "SHT_NULL";
		break;
	case CELF::SHT_PROGBITS:
		sTemp = "SHT_PROGBITS";
		break;
	case CELF::SHT_SYMTAB:
		sTemp = "SHT_SYMTAB";
		break;
	case CELF::SHT_STRTAB:
		sTemp = "SHT_STRTAB";
		break;
	case CELF::SHT_HASH:
		sTemp = "SHT_HASH";
		break;
	case CELF::SHT_DYNAMIC:
		sTemp = "SHT_DYNAMIC";
		break;
	case CELF::SHT_NOTE:
		sTemp = "SHT_NOTE";
		break;
	case CELF::SHT_NOBITS:
		sTemp = "SHT_NOBITS";
		break;
	case CELF::SHT_REL:
		sTemp = "SHT_REL";
		break;
	case CELF::SHT_DYNSYM:
		sTemp = "SHT_DYNSYM";
		break;
	default:
		sTemp = string_format("Unknown (0x%08X)", pH->nType);
		break;
	}
	m_editFields[i++]->setText(sTemp.c_str());

	sTemp = string_format("0x%08X", pH->nFlags);
	if(pH->nFlags & 0x7)
	{
		sTemp += " (";
		if(pH->nFlags & 0x01)
		{
			sTemp += "SHF_WRITE";
			if(pH->nFlags & 0x6)
			{
				sTemp += " | ";
			}
		}
		if(pH->nFlags & 0x2)
		{
			sTemp += "SHF_ALLOC";
			if(pH->nFlags & 0x04)
			{
				sTemp += " | ";
			}
		}
		if(pH->nFlags & 0x04)
		{
			sTemp += "SHF_EXECINSTR";
		}
		sTemp += ")";
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

	if(pH->nType == CELF::SHT_DYNAMIC)
	{
		FillDynamicSectionListView(section);
		m_dynSecTableWidget->show();
		m_model->SetData(nullptr, 0);
		m_memView->hide();
	}
	else if(pH->nType != CELF::SHT_NOBITS)
	{
		uint8* data = (uint8*) m_pELF->GetSectionData(section);
		auto getByte = [data](uint32 offset)
		{
			return data[offset];
		};
		m_model->SetData(getByte, pH->nSize);
		m_memView->show();
		m_dynSecTableWidget->hide();
	}
	else
	{
		m_model->SetData(nullptr, 0);
		m_memView->show();
		m_dynSecTableWidget->hide();
	}
}

void CELFSectionView::FillDynamicSectionListView(int section)
{
	m_dynSecTableWidget->setRowCount(0);

	const ELFSECTIONHEADER* pH = m_pELF->GetSection(section);
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
		case CELF::DT_NEEDED:
			tempTag = "DT_NEEDED";
			tempVal = stringTable + value;
			break;
		case CELF::DT_PLTRELSZ:
			tempTag = "DT_PLTRELSZ";
			tempVal = string_format("0x%08X", value);
			break;
		case CELF::DT_PLTGOT:
			tempTag = "DT_PLTGOT";
			tempVal = string_format("0x%08X", value);
			break;
		case CELF::DT_HASH:
			tempTag = "DT_HASH";
			tempVal = string_format("0x%08X", value);
			break;
		case CELF::DT_STRTAB:
			tempTag = "DT_STRTAB";
			tempVal = string_format("0x%08X", value);
			break;
		case CELF::DT_SYMTAB:
			tempTag = "DT_SYMTAB";
			tempVal = string_format("0x%08X", value);
			break;
		case CELF::DT_SONAME:
			tempTag = "DT_SONAME";
			tempVal = stringTable + value;
			break;
		case CELF::DT_SYMBOLIC:
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

