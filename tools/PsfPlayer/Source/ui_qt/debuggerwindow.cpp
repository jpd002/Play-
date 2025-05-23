#include "debuggerwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include "string_format.h"
#include "ui_debuggerwindow.h"
#include "DebugSupportSettings.h"
#include "AppConfig.h"
#include "PsfVm.h"
#include "DebugView.h"
#include "DebugUtils.h"

DebuggerWindow::DebuggerWindow(CPsfVm& virtualMachine, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::DebuggerWindow)
    , m_virtualMachine(virtualMachine)
{
	ui->setupUi(this);

	CDebugSupportSettings::GetInstance().Initialize(&CAppConfig::GetInstance());

	m_functionsView = new CFunctionsView(ui->mdiArea);
	m_OnFunctionDblClickConnection = m_functionsView->OnItemDblClick.Connect(std::bind(&DebuggerWindow::OnFunctionsViewFunctionDblClick, this, std::placeholders::_1));
	m_OnFunctionsStateChangeConnection = m_functionsView->OnStateChange.Connect(std::bind(&DebuggerWindow::OnFunctionsViewFunctionsStateChange, this));

	m_addressListView = new CAddressListViewWnd(ui->mdiArea);
	m_addressListView->hide();
	m_addressSelectedConnection = m_addressListView->AddressSelected.Connect([&](uint32 address) { OnFindCallersAddressDblClick(address); });

	m_OnMachineStateChangeConnection = m_virtualMachine.OnMachineStateChange.Connect(std::bind(&DebuggerWindow::OnMachineStateChange, this));
	m_OnRunningStateChangeConnection = m_virtualMachine.OnRunningStateChange.Connect(std::bind(&DebuggerWindow::OnRunningStateChange, this));

	connect(this, &DebuggerWindow::OnMachineStateChange, this, &DebuggerWindow::OnMachineStateChangeMsg);
	connect(this, &DebuggerWindow::OnRunningStateChange, this, &DebuggerWindow::OnRunningStateChangeMsg);
}

DebuggerWindow::~DebuggerWindow()
{
	delete ui;
}

void DebuggerWindow::Reset()
{
	auto debugInfo = m_virtualMachine.GetDebugInfo();
	m_debugView = std::make_unique<CDebugView>(this, ui->mdiArea, m_virtualMachine, &debugInfo.GetCpu(),
	                                           debugInfo.Step, debugInfo.biosDebugInfoProvider, "CPU", 0x400000);

	m_findCallersRequestConnection = m_debugView->GetDisassemblyWindow()->FindCallersRequested.Connect(
	    [&](uint32 address) { OnFindCallersRequested(address); });

	m_functionsView->SetContext(&debugInfo.GetCpu(), debugInfo.biosDebugInfoProvider);

	Layout1600x1200();
}

void DebuggerWindow::on_actionStep_CPU_triggered()
{
	if(!m_debugView) return;
	m_debugView->Step();
}

void DebuggerWindow::OnMachineStateChangeMsg()
{
	if(!m_debugView) return;
	m_debugView->HandleMachineStateChange();
}

void DebuggerWindow::OnRunningStateChangeMsg()
{
	if(!m_debugView) return;
	auto newState = m_virtualMachine.GetStatus();
	m_debugView->HandleRunningStateChange(newState);
}

void DebuggerWindow::OnFunctionsViewFunctionDblClick(uint32 address)
{
	if(!m_debugView) return;
	m_debugView->GetDisassemblyWindow()->SetAddress(address);
}

void DebuggerWindow::OnFunctionsViewFunctionsStateChange()
{
	if(!m_debugView) return;
	m_debugView->GetDisassemblyWindow()->HandleMachineStateChange();
	m_debugView->GetCallStackWindow()->HandleMachineStateChange();
}

void DebuggerWindow::FindWordValue(uint32 mask)
{
	if(!m_debugView) return;

	uint32 targetValue = 0;
	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("Find Value in Memory"),
		                                    tr("Enter value to find:"), QLineEdit::Normal,
		                                    tr("00000000"), &ok);
		if(!ok || res.isEmpty())
			return;

		if(sscanf(res.toStdString().c_str(), "%x", &targetValue) <= 0)
		{
			QMessageBox msgBox;
			msgBox.setText("Invalid hex value.");
			msgBox.exec();
			return;
		}
	}
	auto context = m_debugView->GetContext();
	auto title = string_format("Search results for 0x%08X", targetValue);
	auto refs = DebugUtils::FindWordValueRefs(context, targetValue & mask, mask);

	m_addressListView->SetTitle(std::move(title));
	m_addressListView->SetAddressList(std::move(refs));
	m_addressListView->show();
	m_addressListView->setFocus(Qt::ActiveWindowFocusReason);
}

void DebuggerWindow::OnFindCallersRequested(uint32 address)
{
	auto context = m_debugView->GetContext();
	auto callers = DebugUtils::FindCallers(context, address);
	auto title =
	    [&]() {
		    auto functionName = context->m_Functions.Find(address);
		    if(functionName)
		    {
			    return string_format("Find Callers For '%s' (0x%08X)",
			                         functionName, address);
		    }
		    else
		    {
			    return string_format("Find Callers For 0x%08X", address);
		    }
	    }();

	m_addressListView->SetAddressList(std::move(callers));
	m_addressListView->SetTitle(std::move(title));
	m_addressListView->show();
	m_addressListView->setFocus(Qt::ActiveWindowFocusReason);
}

void DebuggerWindow::OnFindCallersAddressDblClick(uint32 address)
{
	auto disAsm = m_debugView->GetDisassemblyWindow();
	disAsm->SetCenterAtAddress(address);
	disAsm->SetSelectedAddress(address);
}

void DebuggerWindow::Layout1600x1200()
{
	static_cast<QWidget*>(m_debugView->GetDisassemblyWindow()->parent())->setGeometry(0, 0, 1094, 725);
	static_cast<QWidget*>(m_debugView->GetDisassemblyWindow()->parent())->show();
	m_debugView->GetDisassemblyWindow()->show();

	static_cast<QWidget*>(m_debugView->GetRegisterViewWindow()->parent())->setGeometry(1094, 0, 506, 725);
	static_cast<QWidget*>(m_debugView->GetRegisterViewWindow()->parent())->show();
	m_debugView->GetRegisterViewWindow()->show();

	static_cast<QWidget*>(m_debugView->GetMemoryViewWindow()->parent())->setGeometry(0, 725, 1094, 407);
	static_cast<QWidget*>(m_debugView->GetMemoryViewWindow()->parent())->show();
	m_debugView->GetMemoryViewWindow()->show();

	static_cast<QWidget*>(m_debugView->GetCallStackWindow()->parent())->setGeometry(1094, 725, 506, 407);
	static_cast<QWidget*>(m_debugView->GetCallStackWindow()->parent())->show();
	m_debugView->GetCallStackWindow()->show();
}

void DebuggerWindow::on_actionfind_word_value_triggered()
{
	FindWordValue(~0);
}

void DebuggerWindow::on_actionFind_Word_Half_Value_triggered()
{
	FindWordValue(0xFFFF);
}
