#include <stdio.h>
#include "MemoryViewMIPS.h"
#include "win32/InputBox.h"
#include "PS2VM.h"

#define ID_MEMORYVIEW_GOTOADDRESS	40001

using namespace Framework;

CMemoryViewMIPS::CMemoryViewMIPS(HWND hParent, RECT* pR, CMIPS* pCtx) :
CMemoryView(hParent, pR)
{
	m_pCtx = pCtx;

	SetMemorySize(0x02004000);

	m_pOnMachineStateChangeHandler = new CEventHandlerMethod<CMemoryViewMIPS, int>(this, &CMemoryViewMIPS::OnMachineStateChange);

	CPS2VM::m_OnMachineStateChange.InsertHandler(m_pOnMachineStateChangeHandler);
}

CMemoryViewMIPS::~CMemoryViewMIPS()
{
	CPS2VM::m_OnMachineStateChange.RemoveHandler(m_pOnMachineStateChangeHandler);
}

long CMemoryViewMIPS::OnRightButtonUp(int nX, int nY)
{
	POINT pt;
	HMENU hMenu;

	pt.x = nX;
	pt.y = nY;
	ClientToScreen(m_hWnd, &pt);

	hMenu = CreatePopupMenu();
	InsertMenu(hMenu, 0, MF_BYPOSITION, ID_MEMORYVIEW_GOTOADDRESS, _X("Goto Address..."));
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
	return CreateFont(-11, 0, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 49, _X("Courier New"));
}

uint8 CMemoryViewMIPS::GetByte(uint32 nAddress)
{
	return m_pCtx->m_pMemoryMap->GetByte(nAddress);
}

void CMemoryViewMIPS::OnMachineStateChange(int nDummy)
{
	Redraw();
}

void CMemoryViewMIPS::GotoAddress()
{
	uint32 nAddress;
	const xchar* sValue;

	if(CPS2VM::m_nStatus == PS2VM_STATUS_RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	CInputBox i(_X("Goto Address"), _X("Enter new address:"), _X("00000000"));
	sValue = i.GetValue(m_hWnd);

	if(sValue != NULL)
	{
		xsscanf(sValue, _X("%x"), &nAddress);
		ScrollToAddress(nAddress);
	}
}
