#include <stdio.h>
#include <boost/bind.hpp>
#include "MemoryViewMIPS.h"
#include "win32/InputBox.h"
#include "lexical_cast_ex.h"
#include "DebugExpressionEvaluator.h"
#include "string_cast.h"

#define ID_MEMORYVIEW_GOTOADDRESS		40001
#define ID_MEMORYVIEW_FOLLOWPOINTER		40002

CMemoryViewMIPS::CMemoryViewMIPS(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* pCtx)
: CMemoryView(hParent, rect)
, m_virtualMachine(virtualMachine)
, m_pCtx(pCtx)
{
	SetMemorySize(0x02004000);

	m_virtualMachine.OnMachineStateChange.connect(boost::bind(&CMemoryViewMIPS::OnMachineStateChange, this));
}

CMemoryViewMIPS::~CMemoryViewMIPS()
{

}

long CMemoryViewMIPS::OnRightButtonUp(int nX, int nY)
{
	POINT pt;
	pt.x = nX;
	pt.y = nY;
	ClientToScreen(m_hWnd, &pt);

	HMENU hMenu = CreatePopupMenu();
	InsertMenu(hMenu, 0, MF_BYPOSITION, ID_MEMORYVIEW_GOTOADDRESS, _T("Goto Address..."));

	//Follow pointer
	{
		uint32 selection = GetSelection();
		if((selection & 0x03) == 0)
		{
			uint32 valueAtSelection = m_pCtx->m_pMemoryMap->GetWord(GetSelection());
			std::tstring followPointerText = _T("Follow Pointer (0x") + lexical_cast_hex<std::tstring>(valueAtSelection, 8) + _T(")");
			InsertMenu(hMenu, 1, MF_BYPOSITION, ID_MEMORYVIEW_FOLLOWPOINTER, followPointerText.c_str());
		}
	}

	TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_hWnd, NULL); 

	return FALSE;
}

long CMemoryViewMIPS::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	switch(nID)
	{
	case ID_MEMORYVIEW_GOTOADDRESS:
		GotoAddress();
		break;
	case ID_MEMORYVIEW_FOLLOWPOINTER:
		FollowPointer();
		break;
	}
	return TRUE;
}

HFONT CMemoryViewMIPS::GetFont()
{
	return CreateFont(-11, 0, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 49, _T("Courier New"));
}

uint8 CMemoryViewMIPS::GetByte(uint32 nAddress)
{
	return m_pCtx->m_pMemoryMap->GetByte(nAddress);
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
			uint32 nAddress = CDebugExpressionEvaluator::Evaluate(string_cast<std::string>(sValue).c_str(), m_pCtx);
			ScrollToAddress(nAddress);
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

	uint32 valueAtSelection = m_pCtx->m_pMemoryMap->GetWord(GetSelection());
	ScrollToAddress(valueAtSelection);
	SetSelectionStart(valueAtSelection);
}
