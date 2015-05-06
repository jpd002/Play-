#include <boost/bind.hpp>
#include "win32/InputBox.h"
#include "win32/Font.h"
#include "string_cast.h"
#include "string_format.h"
#include "MemoryViewMIPS.h"
#include "DebugExpressionEvaluator.h"

CMemoryViewMIPS::CMemoryViewMIPS(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* context)
: CMemoryView(hParent, rect)
, m_virtualMachine(virtualMachine)
, m_context(context)
{
	m_font = Framework::Win32::CreateFont(_T("Courier New"), 8);

	SetMemorySize(0x02004000);

	m_virtualMachine.OnMachineStateChange.connect(boost::bind(&CMemoryViewMIPS::OnMachineStateChange, this));
}

CMemoryViewMIPS::~CMemoryViewMIPS()
{

}

long CMemoryViewMIPS::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	switch(nID)
	{
	case ID_MEMORYVIEWMIPS_GOTOADDRESS:
		GotoAddress();
		break;
	case ID_MEMORYVIEWMIPS_FOLLOWPOINTER:
		FollowPointer();
		break;
	}
	return CMemoryView::OnCommand(nID, nCmd, hSender);
}

uint8 CMemoryViewMIPS::GetByte(uint32 nAddress)
{
	return m_context->m_pMemoryMap->GetByte(nAddress);
}

HMENU CMemoryViewMIPS::CreateContextualMenu()
{
	HMENU menu = CMemoryView::CreateContextualMenu();

	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	AppendMenu(menu, MF_STRING, ID_MEMORYVIEWMIPS_GOTOADDRESS, _T("Goto Address..."));

	//Follow pointer
	{
		uint32 selection = GetSelection();
		if((selection & 0x03) == 0)
		{
			uint32 valueAtSelection = m_context->m_pMemoryMap->GetWord(GetSelection());
			auto followPointerText = string_format(_T("Follow Pointer (0x%0.8X)"), valueAtSelection);
			AppendMenu(menu, MF_STRING, ID_MEMORYVIEWMIPS_FOLLOWPOINTER, followPointerText.c_str());
		}
	}

	return menu;
}

void CMemoryViewMIPS::OnMachineStateChange()
{
	Redraw();
}

void CMemoryViewMIPS::GotoAddress()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	Framework::Win32::CInputBox i(_T("Goto Address"), _T("Enter new address:"), _T("00000000"));
	const TCHAR* sValue = i.GetValue(m_hWnd);

	if(sValue != NULL)
	{
		try
		{
			uint32 nAddress = CDebugExpressionEvaluator::Evaluate(string_cast<std::string>(sValue).c_str(), m_context);
			SetSelectionStart(nAddress);
		}
		catch(const std::exception& exception)
		{
			std::tstring message = std::tstring(_T("Error evaluating expression: ")) + string_cast<std::tstring>(exception.what());
			MessageBox(m_hWnd, message.c_str(), NULL, 16);
		}
	}
}

void CMemoryViewMIPS::FollowPointer()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	uint32 valueAtSelection = m_context->m_pMemoryMap->GetWord(GetSelection());
	SetSelectionStart(valueAtSelection);
}
