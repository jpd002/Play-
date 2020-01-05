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
#include "DebugExpressionEvaluator.h"

CMemoryViewMIPSWnd::CMemoryViewMIPSWnd(QMdiArea* parent, CVirtualMachine& virtualMachine, CMIPS* ctx, int size)
    : QMdiSubWindow(parent)
    , m_tableView(new CMemoryViewTable(this, &virtualMachine, ctx, 0, true))
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
	auto getByte = [ctx](uint32 address) {
		return ctx->m_pMemoryMap->GetByte(address);
	};

	m_tableView->SetData(getByte, size);

	UpdateStatusBar(0);
	m_OnSelectionChangeConnection = m_tableView->OnSelectionChange.Connect(std::bind(&CMemoryViewMIPSWnd::UpdateStatusBar, this, std::placeholders::_1));
}

CMemoryViewMIPSWnd::~CMemoryViewMIPSWnd()
{
}

void CMemoryViewMIPSWnd::showEvent(QShowEvent* evt)
{
	QMdiSubWindow::showEvent(evt);
	m_tableView->ShowEvent();
}

void CMemoryViewMIPSWnd::resizeEvent(QResizeEvent* evt)
{
	QMdiSubWindow::resizeEvent(evt);
	m_tableView->ResizeEvent();
}

void CMemoryViewMIPSWnd::HandleMachineStateChange()
{
	m_tableView->HandleMachineStateChange();
}

int CMemoryViewMIPSWnd::GetBytesPerLine()
{
	return m_tableView->GetBytesPerLine();
}

void CMemoryViewMIPSWnd::SetBytesPerLine(int bytesForLine)
{
	m_tableView->SetBytesPerLine(bytesForLine);
}

void CMemoryViewMIPSWnd::UpdateStatusBar(uint32 address)
{
	auto caption = string_format("Address : 0x%08X", address);
	m_addressEdit->setText(caption.c_str());
}
