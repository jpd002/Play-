#include <QAction>
#include <QApplication>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QVBoxLayout>

#include "string_format.h"
#include "MemoryViewMIPSWnd.h"
#include "ui_MemoryViewMIPSWnd.h"
#include "DebugExpressionEvaluator.h"

CMemoryViewMIPSWnd::CMemoryViewMIPSWnd(QWidget* parent, CVirtualMachine& virtualMachine, CMIPS* ctx, int size)
    : QWidget(parent)
    , ui(new Ui::CMemoryViewMIPSWnd)
{
	ui->setupUi(this);

	auto getByte = [ctx](uint32 address) {
		return ctx->m_pMemoryMap->GetByte(address);
	};

	ui->tableView->Setup(&virtualMachine, ctx, true);
	ui->tableView->SetData(getByte, size);

	UpdateStatusBar(0);
	m_OnSelectionChangeConnection = ui->tableView->OnSelectionChange.Connect(std::bind(&CMemoryViewMIPSWnd::UpdateStatusBar, this, std::placeholders::_1));
}

CMemoryViewMIPSWnd::~CMemoryViewMIPSWnd()
{
	delete ui;
}

void CMemoryViewMIPSWnd::showEvent(QShowEvent* evt)
{
	QWidget::showEvent(evt);
	ui->tableView->ShowEvent();
}

void CMemoryViewMIPSWnd::resizeEvent(QResizeEvent* evt)
{
	QWidget::resizeEvent(evt);
	ui->tableView->ResizeEvent();
}

void CMemoryViewMIPSWnd::HandleMachineStateChange()
{
	ui->tableView->HandleMachineStateChange();
}

int CMemoryViewMIPSWnd::GetBytesPerLine()
{
	return ui->tableView->GetBytesPerLine();
}

void CMemoryViewMIPSWnd::SetBytesPerLine(int bytesForLine)
{
	ui->tableView->SetBytesPerLine(bytesForLine);
}

void CMemoryViewMIPSWnd::SetAddress(uint32 address)
{
	ui->tableView->SetSelectionStart(address);
}

void CMemoryViewMIPSWnd::UpdateStatusBar(uint32 address)
{
	auto caption = string_format("Address : 0x%08X", address);
	ui->addressEdit->setText(caption.c_str());
}
