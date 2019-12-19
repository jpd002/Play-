#include <QAction>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QVBoxLayout>

#include "string_format.h"
#include "MemoryViewMIPSWnd.h"
#include "DebugExpressionEvaluator.h"

CMemoryViewMIPSWnd::CMemoryViewMIPSWnd(QMdiArea* parent, CVirtualMachine& virtualMachine, CMIPS* ctx, int size)
    : QMdiSubWindow(parent)
    , m_virtualMachine(virtualMachine)
    , m_tableView(new QTableView(this))
    , m_context(ctx)
    , m_model(new CQtMemoryViewMIPSModel(this, ctx, size))
{
	resize(320, 240);
	parent->addSubWindow(this);
	setWindowTitle("Memory");

	auto centralwidget = new QWidget(this);
	auto verticalLayout = new QVBoxLayout(centralwidget);

	m_addressEdit = new QLineEdit(centralwidget);
	m_addressEdit->setReadOnly(true);

	verticalLayout->addWidget(m_addressEdit);
	verticalLayout->addWidget(m_tableView);

	setWidget(centralwidget);
	m_tableView->setModel(m_model);

	// prepare monospaced font
	QFont font = m_tableView->font();
	QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	fixedFont.setPointSize(font.pointSize());
	m_tableView->setFont(fixedFont);

	QFontMetrics metric(fixedFont);
	m_cwidth = metric.maxWidth();

	auto header = m_tableView->horizontalHeader();
	header->setMinimumSectionSize(m_cwidth);
	header->hide();
	ResizeColumns();

	m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_tableView, &QTableView::customContextMenuRequested, this, &CMemoryViewMIPSWnd::ShowMenu);
	connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMemoryViewMIPSWnd::SelectionChanged);

	UpdateStatusBar();
}

CMemoryViewMIPSWnd::~CMemoryViewMIPSWnd()
{
}

void CMemoryViewMIPSWnd::showEvent(QShowEvent* evt)
{
	QMdiSubWindow::showEvent(evt);
	AutoColumn();
}

void CMemoryViewMIPSWnd::resizeEvent(QResizeEvent* evt)
{
	QMdiSubWindow::resizeEvent(evt);
	if(!m_model || m_bytesPerLine)
		return;

	AutoColumn();
}

void CMemoryViewMIPSWnd::HandleMachineStateChange()
{
	m_model->Redraw();
}

int CMemoryViewMIPSWnd::GetBytesPerLine()
{
	return m_bytesPerLine;
}

void CMemoryViewMIPSWnd::SetBytesPerLine(int bytesForLine)
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

void CMemoryViewMIPSWnd::UpdateStatusBar()
{
	auto caption = string_format("Address : 0x%08X", m_selected);
	m_addressEdit->setText(caption.c_str());
}

void CMemoryViewMIPSWnd::ShowMenu(const QPoint& pos)
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
		connect(autoAction, &QAction::triggered, std::bind(&CMemoryViewMIPSWnd::SetBytesPerLine, this, 0));

		auto byte16Action = bytesPerLineMenu->addAction("16 bytes");
		byte16Action->setChecked(m_bytesPerLine == 16);
		byte16Action->setCheckable(true);
		bytesPerLineActionGroup->addAction(byte16Action);
		connect(byte16Action, &QAction::triggered, std::bind(&CMemoryViewMIPSWnd::SetBytesPerLine, this, 16));

		auto byte32Action = bytesPerLineMenu->addAction("32 bytes");
		byte32Action->setChecked(m_bytesPerLine == 32);
		byte32Action->setCheckable(true);
		bytesPerLineActionGroup->addAction(byte32Action);
		connect(byte32Action, &QAction::triggered, std::bind(&CMemoryViewMIPSWnd::SetBytesPerLine, this, 32));
	}

	{
		auto unitMenu = rightClickMenu->addMenu(tr("&Display Unit"));
		auto unitActionGroup = new QActionGroup(this);
		unitActionGroup->setExclusive(true);

		auto activeUnit = m_model->GetActiveUnit();
		for(uint32 i = 0; i < CQtMemoryViewMIPSModel::g_units.size(); i++)
		{
			const auto& unit = CQtMemoryViewMIPSModel::g_units[i];
			auto itemAction = unitMenu->addAction(unit.description);
			itemAction->setChecked(i == activeUnit);
			itemAction->setCheckable(true);
			unitActionGroup->addAction(itemAction);
			connect(itemAction, &QAction::triggered, std::bind(&CMemoryViewMIPSWnd::SetActiveUnit, this, i));
		}
	}

	rightClickMenu->addSeparator();
	{
		auto itemAction = rightClickMenu->addAction("Goto Address...");
		connect(itemAction, &QAction::triggered, std::bind(&CMemoryViewMIPSWnd::GotoAddress, this));
	}

	{
		uint32 selection = m_selected;
		if((selection & 0x03) == 0)
		{
			uint32 valueAtSelection = m_context->m_pMemoryMap->GetWord(selection);
			auto followPointerText = string_format("Follow Pointer (0x%08X)", valueAtSelection);
			auto itemAction = rightClickMenu->addAction(followPointerText.c_str());
			connect(itemAction, &QAction::triggered, std::bind(&CMemoryViewMIPSWnd::FollowPointer, this));
		}
	}

	rightClickMenu->popup(m_tableView->viewport()->mapToGlobal(pos));
}

void CMemoryViewMIPSWnd::ResizeColumns()
{
	auto header = m_tableView->horizontalHeader();
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

void CMemoryViewMIPSWnd::AutoColumn()
{
	QFont font = m_tableView->font();
	QFontMetrics metric(font);
	int columnHeaderWidth = metric.horizontalAdvance(" 0x00000000");

	int tableWidth = m_tableView->size().width() - columnHeaderWidth - m_tableView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
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

void CMemoryViewMIPSWnd::GotoAddress()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		// MessageBeep(-1);
		return;
	}

	std::string sValue;
	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("Goto Address"),
		                                    tr("Enter new address:"), QLineEdit::Normal,
		                                    tr("00000000"), &ok);
		if(!ok || res.isEmpty())
			return;

		sValue = res.toStdString();
	}

	try
	{
		uint32 nAddress = CDebugExpressionEvaluator::Evaluate(sValue.c_str(), m_context);
		SetSelectionStart(nAddress);
	}
	catch(const std::exception& exception)
	{
		std::string message = std::string("Error evaluating expression: ") + exception.what();
		QMessageBox::critical(this, tr("Error"), tr(message.c_str()), QMessageBox::Ok, QMessageBox::Ok);
	}
}

void CMemoryViewMIPSWnd::FollowPointer()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		// MessageBeep(-1);
		return;
	}

	uint32 valueAtSelection = m_context->m_pMemoryMap->GetWord(m_selected);
	SetSelectionStart(valueAtSelection);
}

void CMemoryViewMIPSWnd::SetActiveUnit(int index)
{
	m_model->SetActiveUnit(index);
	AutoColumn();
}

void CMemoryViewMIPSWnd::SetSelectionStart(uint32 address)
{
	auto column = address % m_model->BytesForCurrentLine();
	auto row = (address - column) / m_model->UnitsForCurrentLine();

	auto index = m_model->index(row, column);
	m_tableView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
	m_tableView->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void CMemoryViewMIPSWnd::SelectionChanged()
{
	auto indexes = m_tableView->selectionModel()->selectedIndexes();
	if(!indexes.empty())
	{
		auto index = indexes.first();
		int offset = 0;
		if(index.column() < m_model->UnitsForCurrentLine())
		{
			offset = index.column() * m_model->GetBytesPerUnit();
		}
		int address = offset + (index.row() * m_model->BytesForCurrentLine());
		m_selected = address;
	}
	UpdateStatusBar();
}
