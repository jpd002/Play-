#include <stdio.h>
#include <boost/bind.hpp>
#include "MemoryViewMIPS.h"
#include "win32/InputBox.h"
#include "DebugExpressionEvaluator.h"
#include "string_cast.h"

#define ID_MEMORYVIEW_GOTOADDRESS	40001

using namespace Framework;
using namespace boost;

CMemoryViewMIPS::CMemoryViewMIPS(HWND hParent, RECT* pR, CVirtualMachine& virtualMachine, CMIPS* pCtx) :
CMemoryView(hParent, pR),
m_virtualMachine(virtualMachine),
m_pCtx(pCtx)
{
	SetMemorySize(0x02004000);

	m_virtualMachine.OnMachineStateChange.connect(bind(&CMemoryViewMIPS::OnMachineStateChange, this));
}

CMemoryViewMIPS::~CMemoryViewMIPS()
{

}

long CMemoryViewMIPS::OnRightButtonUp(int nX, int nY)
{
	POINT pt;
	HMENU hMenu;

	pt.x = nX;
	pt.y = nY;
	ClientToScreen(m_hWnd, &pt);

	hMenu = CreatePopupMenu();
	InsertMenu(hMenu, 0, MF_BYPOSITION, ID_MEMORYVIEW_GOTOADDRESS, _T("Goto Address..."));
	//Goto register?

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

	Win32::CInputBox i(_T("Goto Address"), _T("Enter new address:"), _T("00000000"));
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
