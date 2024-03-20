#include "MemoryViewTable.h"
#include <cmath>
#include <QHeaderView>
#include <QTextLayout>
#include <QMenu>
#include <QActionGroup>

#include "string_format.h"
#include "DebugUtils.h"

CMemoryViewTable::CMemoryViewTable(QWidget* parent)
    : QTableView(parent)
{
	m_model = new CQtMemoryViewModel(this);

	setModel(m_model);
	SetBytesPerLine(32);

	auto fixedFont = DebugUtils::CreateMonospaceFont();
	setFont(fixedFont);

	QFontMetrics metric(fixedFont);
	m_cwidth = metric.maxWidth();

	auto header = horizontalHeader();
	header->setMinimumSectionSize(1);
	header->setStretchLastSection(true);
	header->setSectionResizeMode(QHeaderView::ResizeToContents);
	header->hide();
	m_model->Redraw();

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QTableView::customContextMenuRequested, this, &CMemoryViewTable::ShowContextMenu);
	connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMemoryViewTable::SelectionChanged);
}

int CMemoryViewTable::sizeHintForColumn(int col) const
{
	if(col < m_model->columnCount() - 1)
	{
		return ComputeItemCellWidth();
	}
	return 0;
}

int CMemoryViewTable::ComputeItemCellWidth() const
{
	auto modelString = std::string(m_model->CharsPerUnit(), '0');
	int marginWidth = style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this) + 1;

	QTextLayout layout(modelString.c_str(), font());
	layout.beginLayout();
	auto line = layout.createLine();
	line.setLineWidth(1000);
	int textWidth = ceil(line.naturalTextWidth());
	layout.endLayout();

	int result = textWidth + (2 * marginWidth) + 1;
	return result;
}

void CMemoryViewTable::SetData(CQtMemoryViewModel::getByteProto getByte, uint64 size, uint32 windowSize)
{
	m_model->SetData(getByte, size, windowSize);
}

void CMemoryViewTable::ShowEvent()
{
	AutoColumn();
}

void CMemoryViewTable::ResizeEvent()
{
	if(!m_model || m_bytesPerLine)
		return;

	AutoColumn();
}

int CMemoryViewTable::GetBytesPerLine()
{
	return m_bytesPerLine;
}

void CMemoryViewTable::SetBytesPerLine(int bytesForLine)
{
	m_bytesPerLine = bytesForLine;
	if(bytesForLine)
	{
		m_model->SetBytesPerRow(bytesForLine);
		m_model->Redraw();
	}
	else
	{
		AutoColumn();
	}
}

void CMemoryViewTable::ShowContextMenu(const QPoint& pos)
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
		connect(autoAction, &QAction::triggered, std::bind(&CMemoryViewTable::SetBytesPerLine, this, 0));

		auto byte16Action = bytesPerLineMenu->addAction("16 bytes");
		byte16Action->setChecked(m_bytesPerLine == 16);
		byte16Action->setCheckable(true);
		bytesPerLineActionGroup->addAction(byte16Action);
		connect(byte16Action, &QAction::triggered, std::bind(&CMemoryViewTable::SetBytesPerLine, this, 16));

		auto byte32Action = bytesPerLineMenu->addAction("32 bytes");
		byte32Action->setChecked(m_bytesPerLine == 32);
		byte32Action->setCheckable(true);
		bytesPerLineActionGroup->addAction(byte32Action);
		connect(byte32Action, &QAction::triggered, std::bind(&CMemoryViewTable::SetBytesPerLine, this, 32));
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
			connect(itemAction, &QAction::triggered, std::bind(&CMemoryViewTable::SetActiveUnit, this, i));
		}
	}

	PopulateContextMenu(rightClickMenu);
	rightClickMenu->popup(viewport()->mapToGlobal(pos));
}

void CMemoryViewTable::AutoColumn()
{
	if(m_bytesPerLine)
		return;

	int columnHeaderWidth = verticalHeader()->size().width();
	int columnCellWidth = columnWidth(0);

	int tableWidth = size().width() - columnHeaderWidth - style()->pixelMetric(QStyle::PM_ScrollBarExtent);

	auto bytesPerUnit = m_model->GetBytesPerUnit();
	int asciiCell = m_cwidth * bytesPerUnit;

	int i = 16;
	while(true)
	{
		int valueCellsWidth = i * columnCellWidth;
		int asciiCellsWidth = (i * asciiCell) + (m_cwidth * 2);
		int totalWidth = valueCellsWidth + asciiCellsWidth;
		if(totalWidth > tableWidth)
		{
			--i;
			break;
		}
		++i;
	}
	m_model->SetBytesPerRow(i * bytesPerUnit);
	m_model->Redraw();
}

void CMemoryViewTable::SetActiveUnit(int index)
{
	m_model->SetActiveUnit(index);
	m_model->Redraw();
	AutoColumn();
}

void CMemoryViewTable::SetSelectionStart(uint32 address)
{
	m_model->SetWindowCenter(address);
	m_model->Redraw();
	auto index = m_model->TranslateAddressToModelIndex(address);
	selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
	scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void CMemoryViewTable::SelectionChanged()
{
	auto indexes = selectionModel()->selectedIndexes();
	if(!indexes.empty())
	{
		auto index = indexes.first();
		m_selected = m_model->TranslateModelIndexToAddress(index);
		OnSelectionChange(m_selected);
	}
}
