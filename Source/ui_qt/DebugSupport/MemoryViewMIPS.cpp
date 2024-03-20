#include "MemoryViewMIPS.h"
#include <QMenu>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include "string_format.h"
#include "DebugExpressionEvaluator.h"

CMemoryViewMIPS::CMemoryViewMIPS(QWidget* parent)
    : CMemoryViewTable(parent)
{
}

void CMemoryViewMIPS::SetContext(CVirtualMachine* virtualMachine, CMIPS* ctx)
{
	m_virtualMachine = virtualMachine;
	m_context = ctx;
}

void CMemoryViewMIPS::HandleMachineStateChange()
{
	m_model->Redraw();
}

void CMemoryViewMIPS::SetAddress(uint32 address)
{
	SetSelectionStart(address);
}

void CMemoryViewMIPS::PopulateContextMenu(QMenu* rightClickMenu)
{
	rightClickMenu->addSeparator();

	{
		auto itemAction = rightClickMenu->addAction("Goto Address...");
		connect(itemAction, &QAction::triggered, std::bind(&CMemoryViewMIPS::GotoAddress, this));
	}

	{
		uint32 selection = m_selected;
		if((selection & 0x03) == 0)
		{
			uint32 valueAtSelection = m_context->m_pMemoryMap->GetWord(selection);
			auto followPointerText = string_format("Follow Pointer (0x%08X)", valueAtSelection);
			auto itemAction = rightClickMenu->addAction(followPointerText.c_str());
			connect(itemAction, &QAction::triggered, std::bind(&CMemoryViewMIPS::FollowPointer, this));
		}
	}
}

void CMemoryViewMIPS::GotoAddress()
{
	if(m_virtualMachine->GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
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

void CMemoryViewMIPS::FollowPointer()
{
	if(m_virtualMachine->GetStatus() == CVirtualMachine::RUNNING)
	{
		QApplication::beep();
		return;
	}

	uint32 valueAtSelection = m_context->m_pMemoryMap->GetWord(m_selected);
	SetSelectionStart(valueAtSelection);
}
