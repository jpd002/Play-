#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>
#include <QMenu>
#include <QString>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QTextLayout>
#include <cmath>

#include "DisAsmWnd.h"
#include "ee/VuAnalysis.h"
#include "QtDisAsmTableModel.h"
#include "QtDisAsmVuTableModel.h"

#include "countof.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#include "DebugExpressionEvaluator.h"
#include "DebugUtils.h"

CDisAsmWnd::CDisAsmWnd(QWidget* parent, CVirtualMachine& virtualMachine, CMIPS* ctx, const char* name, CQtDisAsmTableModel::DISASM_TYPE disAsmType)
    : QTableView(parent)
    , m_virtualMachine(virtualMachine)
    , m_ctx(ctx)
    , m_disAsmType(disAsmType)
{
	HistoryReset();

	setFont(DebugUtils::CreateMonospaceFont());

	m_numericalCellWidth = ComputeNumericalCellWidth();

	resize(320, 240);

	switch(disAsmType)
	{
	case CQtDisAsmTableModel::DISASM_STANDARD:
		m_model = new CQtDisAsmTableModel(this, virtualMachine, ctx);
		m_instructionSize = 4;
		break;
	case CQtDisAsmTableModel::DISASM_VU:
		m_model = new CQtDisAsmVuTableModel(this, virtualMachine, ctx);
		m_instructionSize = 8;
		break;
	default:
		assert(0);
		break;
	}

	setModel(m_model);

	auto header = horizontalHeader();
	header->setMinimumSectionSize(1);
	header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(4, QHeaderView::Interactive);
	header->setSectionResizeMode(5, QHeaderView::Interactive);
	if(disAsmType == CQtDisAsmTableModel::DISASM_STANDARD)
	{
		header->setSectionResizeMode(6, QHeaderView::Stretch);
	}
	else
	{
		header->setSectionResizeMode(6, QHeaderView::Interactive);
		header->setSectionResizeMode(7, QHeaderView::Interactive);
		header->setSectionResizeMode(8, QHeaderView::Stretch);
	}

	verticalHeader()->hide();
	resizeColumnsToContents();

	header->resizeSection(4, 80);
	header->resizeSection(5, 160);
	if(disAsmType == CQtDisAsmTableModel::DISASM_VU)
	{
		header->resizeSection(6, 80);
		header->resizeSection(7, 160);
	}

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QTableView::customContextMenuRequested, this, &CDisAsmWnd::ShowContextMenu);

	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::ContiguousSelection);
	connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &CDisAsmWnd::selectionChanged);

	QAction* copyAction = new QAction("copy", this);
	copyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
	copyAction->setShortcutContext(Qt::WidgetShortcut);
	connect(copyAction, &QAction::triggered, this, &CDisAsmWnd::OnCopy);
	addAction(copyAction);

	QAction* breakpointAction = new QAction("breakpoint toggle", this);
	breakpointAction->setShortcut(QKeySequence(Qt::Key_F9));
	connect(breakpointAction, &QAction::triggered, this, &CDisAsmWnd::OnListDblClick);
	addAction(breakpointAction);

	QAction* rightArrowAction = new QAction("Right Arrow", this);
	rightArrowAction->setShortcut(QKeySequence(Qt::Key_Right));
	connect(rightArrowAction, &QAction::triggered, [&]() {
		if(m_selected != MIPS_INVALID_PC)
		{
			uint32 nOpcode = GetInstruction(m_selected);
			if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, m_selected, nOpcode) == MIPS_BRANCH_NORMAL)
			{
				m_address = m_selected; // Ensure history tracks where we came from
				GotoEA();
				SetSelectedAddress(m_address);
			}
			else if(HistoryHasNext())
			{
				HistoryGoForward();
				SetSelectedAddress(m_address);
			}
		}
	});
	addAction(rightArrowAction);

	QAction* leftArrowAction = new QAction("Left Arrow", this);
	leftArrowAction->setShortcut(QKeySequence(Qt::Key_Left));
	connect(leftArrowAction, &QAction::triggered, [&]() {
		if(HistoryHasPrevious())
		{
			HistoryGoBack();
			SetSelectedAddress(m_address);
		}
	});
	addAction(leftArrowAction);
	connect(this, &QTableView::doubleClicked, this, &CDisAsmWnd::OnListDblClick);
}

int CDisAsmWnd::ComputeNumericalCellWidth() const
{
	int marginWidth = style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this) + 1;

	//There's 8 characters in the address and instruction views
	QTextLayout layout("00000000", font());
	layout.beginLayout();
	auto line = layout.createLine();
	line.setLineWidth(1000);
	int textWidth = ceil(line.naturalTextWidth());
	layout.endLayout();

	int result = textWidth + (2 * marginWidth) + 1;
	return result;
}

void CDisAsmWnd::ShowContextMenu(const QPoint& pos)
{
	QMenu* rightClickMenu = new QMenu(this);

	QAction* goToPcAction = new QAction(this);
	goToPcAction->setText("GoTo PC");
	connect(goToPcAction, &QAction::triggered, std::bind(&CDisAsmWnd::GotoPC, this));
	rightClickMenu->addAction(goToPcAction);

	QAction* goToAddrAction = new QAction(this);
	goToAddrAction->setText("Goto Address...");
	connect(goToAddrAction, &QAction::triggered, std::bind(&CDisAsmWnd::GotoAddress, this));
	rightClickMenu->addAction(goToAddrAction);

	QAction* editCommentAction = new QAction(this);
	editCommentAction->setText("Edit Comment...");
	connect(editCommentAction, &QAction::triggered, std::bind(&CDisAsmWnd::EditComment, this));
	rightClickMenu->addAction(editCommentAction);

	QAction* findCallerAction = new QAction(this);
	findCallerAction->setText("Find Callers");
	connect(findCallerAction, &QAction::triggered, std::bind(&CDisAsmWnd::FindCallers, this));
	rightClickMenu->addAction(findCallerAction);

	auto index = currentIndex();
	if(index.isValid())
	{
		if(m_selected != MIPS_INVALID_PC)
		{
			uint32 nOpcode = GetInstruction(m_selected);
			if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, m_selected, nOpcode) == MIPS_BRANCH_NORMAL)
			{
				char sTemp[256];
				uint32 nAddress = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, m_selected, nOpcode);
				if(nAddress != MIPS_INVALID_PC)
				{
					snprintf(sTemp, countof(sTemp), ("Go to 0x%08X"), nAddress);
					QAction* goToEaAction = new QAction(this);
					goToEaAction->setText(sTemp);
					connect(goToEaAction, &QAction::triggered, std::bind(&CDisAsmWnd::GotoEA, this));
					rightClickMenu->addAction(goToEaAction);
				}
			}
		}
	}

	if(HistoryHasPrevious())
	{
		char sTemp[256];
		snprintf(sTemp, countof(sTemp), ("Go back (0x%08X)"), HistoryGetPrevious());
		QAction* goToEaAction = new QAction(this);
		goToEaAction->setText(sTemp);
		connect(goToEaAction, &QAction::triggered, std::bind(&CDisAsmWnd::HistoryGoBack, this));
		rightClickMenu->addAction(goToEaAction);
	}

	if(HistoryHasNext())
	{
		char sTemp[256];
		snprintf(sTemp, countof(sTemp), ("Go forward (0x%08X)"), HistoryGetNext());
		QAction* goToEaAction = new QAction(this);
		goToEaAction->setText(sTemp);
		connect(goToEaAction, &QAction::triggered, std::bind(&CDisAsmWnd::HistoryGoForward, this));
		rightClickMenu->addAction(goToEaAction);
	}

	if(m_disAsmType == CQtDisAsmTableModel::DISASM_VU)
	{
		QAction* analyseVuction = new QAction(this);
		analyseVuction->setText("Analyse Microprogram");
		connect(analyseVuction, &QAction::triggered,
		        [&]() {
			        CVuAnalysis::Analyse(m_ctx, 0, 0x4000);
			        m_model->Redraw();
		        });
		rightClickMenu->addAction(analyseVuction);
	}

	rightClickMenu->popup(viewport()->mapToGlobal(pos));
}

bool CDisAsmWnd::isAddressInView(QModelIndex& index) const
{
	auto startRow = indexAt(rect().topLeft());
	auto endRow = indexAt(rect().bottomRight());
	return index.row() >= startRow.row() && index.row() <= endRow.row();
}

int CDisAsmWnd::sizeHintForColumn(int col) const
{
	if(col == 0 || col == 2)
	{
		//The added units is to account for margins
		return m_model->GetLinePixMapWidth() + 11;
	}
	else
	{
		return m_numericalCellWidth;
	}
}

void CDisAsmWnd::verticalScrollbarValueChanged(int value)
{
	auto topLeftIndex = indexAt(rect().topLeft());
	m_address = m_model->TranslateModelIndexToAddress(topLeftIndex);
	QTableView::verticalScrollbarValueChanged(value);
}

void CDisAsmWnd::SetAddress(uint32 address)
{
	auto addressRow = m_model->TranslateAddressToModelIndex(address);
	if(!isAddressInView(addressRow))
		scrollTo(addressRow, QAbstractItemView::PositionAtTop);
	m_address = address;
}

void CDisAsmWnd::SetCenterAtAddress(uint32 address)
{
	auto addressRow = m_model->TranslateAddressToModelIndex(address);
	if(!isAddressInView(addressRow))
		scrollTo(addressRow, QAbstractItemView::PositionAtCenter);

	m_address = address;
}

void CDisAsmWnd::SetSelectedAddress(uint32 address)
{
	m_selectionEnd = -1;
	m_selected = address;
	auto index = m_model->TranslateAddressToModelIndex(address);
	if(!isAddressInView(index))
		scrollTo(index, QAbstractItemView::PositionAtTop);
	setCurrentIndex(index);
}

void CDisAsmWnd::HandleMachineStateChange()
{
	m_model->Redraw();
}

void CDisAsmWnd::HandleRunningStateChange(CVirtualMachine::STATUS newState)
{
	if(newState == CVirtualMachine::STATUS::PAUSED)
	{
		//Recenter view if we just got into paused state
		m_address = m_ctx->m_State.nPC & ~(m_instructionSize - 1);
		SetCenterAtAddress(m_address);
	}
	m_model->Redraw();
}

void CDisAsmWnd::GotoAddress()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
		return;
	}

	bool ok;
	QString sValue = QInputDialog::getText(this, ("Goto Address"),
	                                       ("Enter new address:"), QLineEdit::Normal,
	                                       ((("0x") + lexical_cast_hex<std::string>(m_address, 8)).c_str()), &ok);
	if(!ok || sValue.isEmpty())
		return;

	{
		try
		{
			uint32 nAddress = CDebugExpressionEvaluator::Evaluate(sValue.toStdString().c_str(), m_ctx);
			if(nAddress & (m_instructionSize - 1))
			{
				QMessageBox::warning(this, tr("Warning"),
				                     tr("Invalid address"),
				                     QMessageBox::Ok, QMessageBox::Ok);
				return;
			}

			if(m_address != nAddress)
			{
				HistorySave(m_address);
			}

			m_address = nAddress;
			SetAddress(nAddress);
		}
		catch(const std::exception& exception)
		{
			std::string message = std::string("Error evaluating expression: ") + exception.what();
			QMessageBox::critical(this, tr("Error"),
			                      tr(message.c_str()),
			                      QMessageBox::Ok, QMessageBox::Ok);
		}
	}
}

void CDisAsmWnd::GotoPC()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
		return;
	}

	m_address = m_ctx->m_State.nPC;
	SetAddress(m_ctx->m_State.nPC);
}

void CDisAsmWnd::GotoEA()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
		return;
	}
	uint32 nOpcode = GetInstruction(m_selected);
	if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, m_selected, nOpcode) == MIPS_BRANCH_NORMAL)
	{
		uint32 nAddress = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, m_selected, nOpcode);
		assert(nAddress != MIPS_INVALID_PC);

		if(m_address != nAddress)
		{
			HistorySave(m_address);
		}

		m_address = nAddress;
		SetAddress(nAddress);
	}
}

void CDisAsmWnd::EditComment()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
		return;
	}

	const char* comment = m_ctx->m_Comments.Find(m_selected);
	std::string commentConv;

	if(comment != nullptr)
	{
		commentConv = comment;
	}
	else
	{
		commentConv = ("");
	}

	bool ok;
	QString value = QInputDialog::getText(this, ("Edit Comment"),
	                                      ("Enter new comment:"), QLineEdit::Normal,
	                                      (commentConv.c_str()), &ok);
	if(!ok)
		return;

	if(!value.isEmpty())
	{
		m_ctx->m_Comments.InsertTag(m_selected, value.toStdString().c_str());
	}
	else
	{
		m_ctx->m_Comments.RemoveTag(m_selected);
	}
	m_ctx->m_Comments.OnTagListChange();
	m_model->Redraw(m_selected);
}

void CDisAsmWnd::FindCallers()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
		return;
	}

	FindCallersRequested(m_selected);
}

CDisAsmWnd::SelectionRangeType CDisAsmWnd::GetSelectionRange()
{
	if(m_selectionEnd == -1)
	{
		return SelectionRangeType(m_selected, m_selected);
	}

	if(m_selectionEnd > m_selected)
	{
		return SelectionRangeType(m_selected, m_selectionEnd);
	}
	else
	{
		return SelectionRangeType(m_selectionEnd, m_selected);
	}
}

void CDisAsmWnd::HistoryReset()
{
	m_historyPosition = -1;
	m_historySize = 0;
	memset(m_history, 0, sizeof(uint32) * HISTORY_STACK_MAX);
}

void CDisAsmWnd::HistorySave(uint32 nAddress)
{
	if(m_historySize == HISTORY_STACK_MAX)
	{
		memmove(m_history + 1, m_history, HISTORY_STACK_MAX - 1);
		m_historySize--;
	}

	m_history[m_historySize] = nAddress;
	m_historyPosition = m_historySize;
	m_historySize++;
}

void CDisAsmWnd::HistoryGoBack()
{
	if(m_historyPosition == -1) return;

	uint32 address = HistoryGetPrevious();
	m_history[m_historyPosition] = m_address;
	m_address = address;

	m_historyPosition--;
	SetAddress(address);
}

void CDisAsmWnd::HistoryGoForward()
{
	if(m_historyPosition == m_historySize) return;

	uint32 address = HistoryGetNext();
	m_historyPosition++;
	m_history[m_historyPosition] = m_address;
	m_address = address;
	SetAddress(address);
}

uint32 CDisAsmWnd::HistoryGetPrevious()
{
	return m_history[m_historyPosition];
}

uint32 CDisAsmWnd::HistoryGetNext()
{
	if(m_historyPosition == m_historySize) return 0;
	return m_history[m_historyPosition + 1];
}

bool CDisAsmWnd::HistoryHasPrevious()
{
	return (m_historySize != 0) && (m_historyPosition != -1);
}

bool CDisAsmWnd::HistoryHasNext()
{
	return (m_historySize != 0) && (m_historyPosition != (m_historySize - 1));
}

uint32 CDisAsmWnd::GetInstruction(uint32 address)
{
	//Address translation perhaps?
	return m_ctx->m_pMemoryMap->GetInstruction(address);
}

void CDisAsmWnd::ToggleBreakpoint(uint32 address)
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
		return;
	}
	m_ctx->ToggleBreakpoint(address);
	m_model->Redraw(address);
}

void CDisAsmWnd::selectionChanged()
{
	auto indexes = selectionModel()->selectedIndexes();
	if(!indexes.empty())
	{
		auto selected = m_model->TranslateModelIndexToAddress(indexes.first());
		auto selectionEnd = m_model->TranslateModelIndexToAddress(indexes.last());
		m_selected = selected;
		m_selectionEnd = (selectionEnd == selected) ? -1 : selectionEnd;
	}
}

void CDisAsmWnd::OnCopy()
{
	std::string text;
	auto selectionRange = GetSelectionRange();
	for(uint32 address = selectionRange.first; address <= selectionRange.second; address = m_model->TranslateAddress(address + m_instructionSize))
	{
		if(address != selectionRange.first)
		{
			text += ("\r\n");
		}
		if(m_disAsmType == CQtDisAsmTableModel::DISASM_STANDARD)
		{
			text += GetInstructionDetailsText(address);
		}
		else
		{
			text += GetInstructionDetailsTextVu(address);
		}
	}

	QApplication::clipboard()->setText(text.c_str());
}

std::string CDisAsmWnd::GetInstructionDetailsText(uint32 address)
{
	uint32 opcode = GetInstruction(address);

	std::string result;

	result += lexical_cast_hex<std::string>(address, 8) + ("    ");
	result += lexical_cast_hex<std::string>(opcode, 8) + ("    ");

	char disasm[256];
	m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address, opcode, disasm, countof(disasm));

	result += disasm;
	for(auto j = strlen(disasm); j < 15; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionOperands(m_ctx, address, opcode, disasm, countof(disasm));
	result += disasm;

	return result;
}

std::string CDisAsmWnd::GetInstructionDetailsTextVu(uint32 address)
{
	uint32 lowerInstruction = GetInstruction(address + 0);
	uint32 upperInstruction = GetInstruction(address + 4);

	std::string result;

	result += lexical_cast_hex<std::string>(address, 8) + ("    ");
	result += lexical_cast_hex<std::string>(upperInstruction, 8) + (" ") + lexical_cast_hex<std::string>(lowerInstruction, 8) + ("    ");

	char disasm[256];

	m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 4, upperInstruction, disasm, countof(disasm));
	result += disasm;

	for(auto j = strlen(disasm); j < 15; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 4, upperInstruction, disasm, countof(disasm));
	result += disasm;

	for(auto j = strlen(disasm); j < 31; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 0, lowerInstruction, disasm, countof(disasm));
	result += disasm;

	for(auto j = strlen(disasm); j < 16; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 0, lowerInstruction, disasm, countof(disasm));
	result += disasm;

	return result;
}

void CDisAsmWnd::OnListDblClick()
{
	ToggleBreakpoint(m_selected);
}
