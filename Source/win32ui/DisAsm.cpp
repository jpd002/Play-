#include <stdio.h>
#include <boost/bind.hpp>
#include "DisAsm.h"
#include "resource.h"
#include "win32/InputBox.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"
#include "WinUtils.h"
#include "../Ps2Const.h"
#include "win32/DeviceContext.h"
#include "win32/ClientDeviceContext.h"
#include "DebugExpressionEvaluator.h"

#define CLSNAME		_T("CDisAsm")
#define YSPACE		3
#define YMARGIN		1

#define ID_DISASM_GOTOPC		40001
#define ID_DISASM_GOTOADDRESS	40002
#define ID_DISASM_GOTOEA		40003
#define ID_DISASM_EDITCOMMENT	40004
#define ID_DISASM_FINDCALLERS	40005
#define ID_DISASM_GOTOPREV		40006
#define ID_DISASM_GOTONEXT		40007

CDisAsm::CDisAsm(HWND hParent, RECT* pR, CVirtualMachine& virtualMachine, CMIPS* pCtx) :
m_virtualMachine(virtualMachine),
m_font(CreateFont(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, _T("Courier New")))
{
	SCROLLINFO si;

	HistoryReset();

	m_nArrow = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ARROW));
	m_nArrowMask = WinUtils::CreateMask(m_nArrow, 0xFF00FF);
	
	m_nBPoint = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BREAKPOINT));
	m_nBPointMask = WinUtils::CreateMask(m_nBPoint, 0xFF00FF);

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= CWindow::WndProc;
		w.lpszClassName	= CLSNAME;
		w.hbrBackground	= NULL;
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		w.style			= CS_DBLCLKS | CS_OWNDC;
		RegisterClassEx(&w);
	}

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_VISIBLE | WS_VSCROLL | WS_CHILD, pR, hParent, NULL);
	SetClassPtr();

	m_virtualMachine.m_OnMachineStateChange.connect(boost::bind(&CDisAsm::OnMachineStateChange, this));
	m_virtualMachine.m_OnRunningStateChange.connect(boost::bind(&CDisAsm::OnRunningStateChange, this));

	m_pCtx = pCtx;

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.nMin = 0;
	si.nMax = 0x4000;
	si.nPos = 0x2000;
	si.fMask = SIF_RANGE | SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, FALSE);

	m_nSelected = 0;
	m_nSelectionEnd = -1;
	m_nAddress = 0;

}

CDisAsm::~CDisAsm()
{
	DeleteObject(m_nArrow);
	DeleteObject(m_nArrowMask);
	DeleteObject(m_nBPoint);
	DeleteObject(m_nBPointMask);
}

void CDisAsm::SetAddress(uint32 nAddress)
{
	m_nAddress = nAddress;
	Redraw();
}

void CDisAsm::OnMachineStateChange()
{
	if(!IsAddressVisible(m_pCtx->m_State.nPC))
	{
		m_nAddress = m_pCtx->m_State.nPC & 0xFFFFFFFC;
	}
	Redraw();
}

void CDisAsm::OnRunningStateChange()
{
	Redraw();
}

void CDisAsm::GotoAddress()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	Framework::Win32::CInputBox i(
		_T("Goto Address"),
		_T("Enter new address:"),
		(_T("0x") + lexical_cast_hex<std::tstring>(m_nAddress, 8)).c_str());

	const TCHAR* sValue = i.GetValue(m_hWnd);
	if(sValue != NULL)
	{
		try
		{
			uint32 nAddress = CDebugExpressionEvaluator::Evaluate(string_cast<std::string>(sValue).c_str(), m_pCtx);
			if(nAddress & 0x03)
			{
				MessageBox(m_hWnd, _T("Invalid address"), NULL, 16);
				return;
			}

			if(m_nAddress != nAddress)
			{
				HistorySave(m_nAddress);
			}

			m_nAddress = nAddress;
			Redraw();
		}
		catch(const std::exception& exception)
		{
			std::tstring message = std::tstring(_T("Error evaluating expression: ")) + string_cast<std::tstring>(exception.what());
			MessageBox(m_hWnd, message.c_str(), NULL, 16);
		}
	}
}

void CDisAsm::GotoPC()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	m_nAddress = m_pCtx->m_State.nPC;
	Redraw();
}

void CDisAsm::GotoEA()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}
	uint32 nOpcode = GetInstruction(m_nSelected);
	if(m_pCtx->m_pArch->IsInstructionBranch(m_pCtx, m_nSelected, nOpcode) == MIPS_BRANCH_NORMAL)
	{
		uint32 nAddress = m_pCtx->m_pArch->GetInstructionEffectiveAddress(m_pCtx, m_nSelected, nOpcode);

		if(m_nAddress != nAddress)
		{
			HistorySave(m_nAddress);
		}

		m_nAddress = nAddress;
		Redraw();
	}
}

void CDisAsm::EditComment()
{
	std::tstring sCommentConv;
	const TCHAR* sValue;
	const char* sComment;

	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	sComment = m_pCtx->m_Comments.Find(m_nSelected);

	if(sComment != NULL)
	{
		sCommentConv = string_cast<std::tstring>(sComment);
	}
	else
	{
		sCommentConv = _T("");
	}

	Framework::Win32::CInputBox i(_T("Edit Comment"), _T("Enter new comment:"), sCommentConv.c_str());
	sValue = i.GetValue(m_hWnd);

	if(sValue != NULL)
	{
		m_pCtx->m_Comments.InsertTag(m_nSelected, string_cast<std::string>(sValue).c_str());
		Redraw();
	}
}

void CDisAsm::FindCallers()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	printf("Searching callers...\r\n");

	for(int i = 0; i < PS2::EERAMSIZE; i += 4)
	{
		uint32 nVal = m_pCtx->m_pMemoryMap->GetInstruction(i);
		if(((nVal & 0xFC000000) == 0x0C000000) || ((nVal & 0xFC000000) == 0x08000000))
		{
			nVal &= 0x3FFFFFF;
			nVal *= 4;
			if(nVal == m_nSelected)
			{
				printf("JAL: 0x%0.8X\r\n", i);
			}
		}
	}
	printf("Done.\r\n");
}

unsigned int CDisAsm::GetFontHeight()
{
	SIZE s;
	Framework::Win32::CClientDeviceContext dc(m_hWnd);

	dc.SelectObject(m_font);

	GetTextExtentPoint32(dc, _T("0"), 1, &s);

	return s.cy;
}

unsigned int CDisAsm::GetLineCount()
{
	unsigned int nFontCY, nLines;
	RECT rwin;

	GetClientRect(&rwin);

	nFontCY = GetFontHeight();

	nLines = (rwin.bottom - (YMARGIN * 2)) / (nFontCY + YSPACE);
	nLines++;

	return nLines;
}

bool CDisAsm::IsAddressVisible(uint32 nAddress)
{
	uint32 nTop = m_nAddress;
	uint32 nBottom = nTop + ((GetLineCount() - 1) * 4);

	if(nAddress < nTop) return false;
	if(nAddress > nBottom) return false;

	return true;
}

CDisAsm::SelectionRangeType CDisAsm::GetSelectionRange()
{
	if(m_nSelectionEnd == -1)
	{
		return SelectionRangeType(m_nSelected, m_nSelected);
	}

	if(m_nSelectionEnd > m_nSelected)
	{
		return SelectionRangeType(m_nSelected, m_nSelectionEnd);
	}
	else
	{
		return SelectionRangeType(m_nSelectionEnd, m_nSelected);
	}
}

void CDisAsm::HistoryReset()
{
	m_nHistoryPosition	= -1;
	m_nHistorySize		= 0;
	memset(m_nHistory, 0, sizeof(uint32) * HISTORY_STACK_MAX);
}

void CDisAsm::HistorySave(uint32 nAddress)
{
	if(m_nHistorySize == HISTORY_STACK_MAX)
	{
		memmove(m_nHistory + 1, m_nHistory, HISTORY_STACK_MAX - 1);
		m_nHistorySize--;
	}

	m_nHistory[m_nHistorySize] = nAddress;
	m_nHistoryPosition = m_nHistorySize;
	m_nHistorySize++;
}

void CDisAsm::HistoryGoBack()
{
	if(m_nHistoryPosition == -1) return;

	uint32 nAddress = HistoryGetPrevious();
	m_nHistory[m_nHistoryPosition] = m_nAddress;
	m_nAddress = nAddress;

	m_nHistoryPosition--;
	Redraw();
}

void CDisAsm::HistoryGoForward()
{
	if(m_nHistoryPosition == m_nHistorySize) return;

	uint32 nAddress = HistoryGetNext();
	m_nHistoryPosition++;
	m_nHistory[m_nHistoryPosition] = m_nAddress;
	m_nAddress = nAddress;

	Redraw();
}

uint32 CDisAsm::HistoryGetPrevious()
{
	return m_nHistory[m_nHistoryPosition];
}

uint32 CDisAsm::HistoryGetNext()
{
	if(m_nHistoryPosition == m_nHistorySize) return 0;
	return m_nHistory[m_nHistoryPosition + 1];
}

bool CDisAsm::HistoryHasPrevious()
{
	return (m_nHistorySize != 0) && (m_nHistoryPosition != -1);
}

bool CDisAsm::HistoryHasNext()
{
	return (m_nHistorySize != 0) && (m_nHistoryPosition != (m_nHistorySize - 1));
}

void CDisAsm::UpdateMouseSelection(unsigned int nX, unsigned int nY)
{
	uint32 nNew;
	if(nX <= 18) return;
	nNew = nY / (GetFontHeight() + YSPACE);
	nNew = (m_nAddress + (nNew * 4));

	if(GetKeyState(VK_SHIFT) & 0x8000)
	{
		m_nSelectionEnd = nNew;
	}
	else
	{
		m_nSelectionEnd = -1;
		if(nNew == m_nSelected) return;
		m_nSelected = nNew;
	}

	Redraw();
}

uint32 CDisAsm::GetAddressAtPosition(unsigned int nX, unsigned int nY)
{
	uint32 nAddress;
	nAddress = nY / (GetFontHeight() + YSPACE);
	nAddress = (m_nAddress + (nAddress * 4));
	return nAddress;
}

uint32 CDisAsm::GetInstruction(uint32 nAddress)
{
	//Address translation perhaps?
	return m_pCtx->m_pMemoryMap->GetInstruction(nAddress);
}

void CDisAsm::ToggleBreakpoint(uint32 nAddress)
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}
	m_pCtx->ToggleBreakpoint(nAddress);
	Redraw();
}

void CDisAsm::UpdatePosition(int nDelta)
{
	m_nAddress += nDelta;
	Redraw();
}

long CDisAsm::OnSetFocus()
{
	if(m_nFocus) return TRUE;
	m_nFocus = true;
	Redraw();
	return TRUE;
}

long CDisAsm::OnKillFocus()
{
	m_nFocus = false;
	Redraw();
	return TRUE;
}

long CDisAsm::OnLeftButtonDown(int nX, int nY)
{
	UpdateMouseSelection(nX, nY);
	SetFocus();
	return FALSE;
}

long CDisAsm::OnRightButtonUp(int nX, int nY)
{
	POINT pt;
	pt.x = nX;
	pt.y = nY;
	ClientToScreen(m_hWnd, &pt);

	unsigned int nPosition = 0;

	HMENU hMenu = CreatePopupMenu();
	InsertMenu(hMenu, nPosition++, MF_BYPOSITION, ID_DISASM_GOTOPC,			_T("Goto PC"));
	InsertMenu(hMenu, nPosition++, MF_BYPOSITION, ID_DISASM_GOTOADDRESS,	_T("Goto Address..."));
	InsertMenu(hMenu, nPosition++, MF_BYPOSITION, ID_DISASM_EDITCOMMENT,	_T("Edit Comment..."));
	InsertMenu(hMenu, nPosition++, MF_BYPOSITION, ID_DISASM_FINDCALLERS,	_T("Find Callers"));

	if(m_nSelected != MIPS_INVALID_PC)
	{
		uint32 nOpcode = GetInstruction(m_nSelected);
		if(m_pCtx->m_pArch->IsInstructionBranch(m_pCtx, m_nSelected, nOpcode) == MIPS_BRANCH_NORMAL)
		{
			TCHAR sTemp[256];
			uint32 nAddress = m_pCtx->m_pArch->GetInstructionEffectiveAddress(m_pCtx, m_nSelected, nOpcode);
			_sntprintf(sTemp, countof(sTemp), _T("Go to 0x%0.8X"), nAddress);
			InsertMenu(hMenu, nPosition++, MF_BYPOSITION, ID_DISASM_GOTOEA, sTemp);
		}
	}

	if(HistoryHasPrevious())
	{
		TCHAR sTemp[256];
		_sntprintf(sTemp, countof(sTemp), _T("Go back (0x%0.8X)"), HistoryGetPrevious());
		InsertMenu(hMenu, nPosition++, MF_BYPOSITION, ID_DISASM_GOTOPREV, sTemp);
	}

	if(HistoryHasNext())
	{
		TCHAR sTemp[256];
		_sntprintf(sTemp, countof(sTemp), _T("Go forward (0x%0.8X)"), HistoryGetNext());
		InsertMenu(hMenu, nPosition++, MF_BYPOSITION, ID_DISASM_GOTONEXT, sTemp);
	}

	TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_hWnd, NULL); 

	return FALSE;
}

long CDisAsm::OnMouseMove(WPARAM nButton, int nX, int nY)
{
	if(!(nButton & MK_LBUTTON)) return TRUE;
	UpdateMouseSelection(nX, nY);
	return FALSE;
}

long CDisAsm::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	CCustomDrawn::OnSize(nType, nX, nY);
	return TRUE;
}

long CDisAsm::OnMouseWheel(short nZ)
{
	if(nZ < 0)
	{
		UpdatePosition(12);
	}
	else
	{
		UpdatePosition(-12);
	}
	return FALSE;
}

long CDisAsm::OnVScroll(unsigned int nType, unsigned int nTrackPos)
{
	switch(nType)
	{
	case SB_LINEDOWN:
		UpdatePosition(4);
		break;
	case SB_LINEUP:
		UpdatePosition(-4);
		break;
	case SB_PAGEDOWN:
		UpdatePosition(40);
		break;
	case SB_PAGEUP:
		UpdatePosition(-40);
		break;
	}
	return FALSE;
}

long CDisAsm::OnKeyDown(unsigned int nKey)
{
	switch(nKey)
	{
	case VK_F9:
		ToggleBreakpoint(m_nSelected);
		break;
	case VK_DOWN:
		m_nSelected += 4;
		if(!IsAddressVisible(m_nSelected))
		{
			UpdatePosition(4);
		}
		else
		{
			Redraw();
		}
		break;
	case VK_UP:
		m_nSelected -= 4;
		if(!IsAddressVisible(m_nSelected))
		{
			UpdatePosition(-4);
		}
		else
		{
			Redraw();
		}
		break;
	}
	return TRUE;
}

long CDisAsm::OnLeftButtonDblClk(int nX, int nY)
{
	if(nX > 18)
	{

	}
	else
	{
		ToggleBreakpoint(GetAddressAtPosition(nX, nY));
	}
	return FALSE;
}

long CDisAsm::OnCommand(unsigned short nID, unsigned short nMsg, HWND hFrom)
{
	switch(nID)
	{
	case ID_DISASM_GOTOPC:
		GotoPC();
		return FALSE;
		break;
	case ID_DISASM_GOTOADDRESS:
		GotoAddress();
		return FALSE;
		break;
	case ID_DISASM_GOTOEA:
		GotoEA();
		return FALSE;
		break;
	case ID_DISASM_EDITCOMMENT:
		EditComment();
		return FALSE;
		break;
	case ID_DISASM_FINDCALLERS:
		FindCallers();
		return FALSE;
		break;
	case ID_DISASM_GOTOPREV:
		HistoryGoBack();
		break;
	case ID_DISASM_GOTONEXT:
		HistoryGoForward();
		break;
	}
	return TRUE;
}

long CDisAsm::OnCopy()
{
	if(!OpenClipboard(m_hWnd))
	{
		return TRUE;
	}

	EmptyClipboard();

	std::tstring sText;

	SelectionRangeType SelectionRange = GetSelectionRange();

	for(uint32 i = SelectionRange.first; i <= SelectionRange.second; i += 4)
	{
		if(i != SelectionRange.first)
		{
			sText += _T("\r\n");
		}

		uint32 nOpcode = GetInstruction(i);

		sText += lexical_cast_hex<std::tstring>(i,       8) + _T("    ");
		sText += lexical_cast_hex<std::tstring>(nOpcode, 8) + _T("    ");

		char sDisAsm[256];
		m_pCtx->m_pArch->GetInstructionMnemonic(m_pCtx, i, nOpcode, sDisAsm, countof(sDisAsm));

		sText += string_cast<std::tstring>(sDisAsm);
		for(size_t j = strlen(sDisAsm); j < 15; j++)
		{
			sText += _T(" ");
		}

		m_pCtx->m_pArch->GetInstructionOperands(m_pCtx, i, nOpcode, sDisAsm, countof(sDisAsm));
		sText += string_cast<std::tstring>(sDisAsm);

	}

	HGLOBAL hMemory = GlobalAlloc(GMEM_MOVEABLE, (sText.length() + 1) * sizeof(TCHAR));
	_tcscpy(reinterpret_cast<TCHAR*>(GlobalLock(hMemory)), sText.c_str());
	GlobalUnlock(hMemory);

#ifdef _UNICODE
	SetClipboardData(CF_UNICODETEXT, hMemory);
#else
	SetClipboardData(CF_TEXT, hMemory);
#endif

	CloseClipboard();

	return TRUE;
}

void CDisAsm::Paint(HDC hDC)
{
	Framework::Win32::CDeviceContext DeviceContext(hDC);

	RECT rwin;
	GetClientRect(&rwin);

	BitBlt(hDC, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);
	SelectObject(hDC, m_font);

	SIZE s;
	GetTextExtentPoint32(hDC, _T("0"), 1, &s);

	int nLines = (rwin.bottom - (YMARGIN * 2)) / (s.cy + YSPACE);
	nLines++;

	RECT rmarg;
	SetRect(&rmarg, 0, 0, 17, rwin.bottom);
	FillRect(hDC, &rmarg, (HBRUSH)COLOR_WINDOW);

	HPEN nLtGrayPen = CreatePen(PS_SOLID, 2, RGB(0x40, 0x40, 0x40));

	HPEN nPen = CreatePen(PS_SOLID, 0, RGB(0x80, 0x80, 0x80));
	SelectObject(hDC, nPen);
	MoveToEx(hDC, 17, 0, NULL);
	LineTo(hDC, 17, rwin.bottom);
	DeleteObject(nPen);

	SetBkMode(hDC, TRANSPARENT);

	unsigned int nY = YMARGIN;

	SelectionRangeType SelectionRange = GetSelectionRange();

	for(int i = 0; i < nLines; i++)
	{
		uint32 nAddress = m_nAddress + (i * 4);
		
		//Not thread safe...?
		if(m_pCtx->m_breakpoints.find(nAddress) != m_pCtx->m_breakpoints.end())
		{
			SetTextColor(hDC, RGB(0x00, 0x00, 0x00));

			{
				HDC hMem = CreateCompatibleDC(hDC);
				SelectObject(hMem, m_nBPointMask);
				BitBlt(hDC, 1, nY + 1, 15, 15, hMem, 0, 0, SRCAND);
				DeleteDC(hMem);
			}

			{
				HDC hMem = CreateCompatibleDC(hDC);
				SelectObject(hMem, m_nBPoint);
				BitBlt(hDC, 1, nY + 1, 15, 15, hMem, 0, 0, SRCPAINT);
				DeleteDC(hMem);
			}
		}

		if(m_virtualMachine.GetStatus() != CVirtualMachine::RUNNING)
		{
			if(nAddress == m_pCtx->m_State.nPC)
			{
				SetTextColor(hDC, RGB(0x00, 0x00, 0x00));

				{
					HDC hMem = CreateCompatibleDC(hDC);
					SelectObject(hMem, m_nArrowMask);
					BitBlt(hDC, 3, nY + 2, 13, 13, hMem, 0, 0, SRCAND);
					DeleteDC(hMem);
				}

				{
					HDC hMem = CreateCompatibleDC(hDC);
					SelectObject(hMem, m_nArrow);
					BitBlt(hDC, 3, nY + 2, 13, 13, hMem, 0, 0, SRCPAINT);
					DeleteDC(hMem);
				}
			}
		}

		if(
			(nAddress >= SelectionRange.first) && 
			(nAddress <= SelectionRange.second)
			)
		{
			RECT rsel;
			SetRect(&rsel, 18, nY, rwin.right, nY + s.cy + YSPACE);
			if(m_nFocus)
			{
				FillRect(hDC, &rsel, (HBRUSH)GetStockObject(BLACK_BRUSH));
			}
			else
			{
				FillRect(hDC, &rsel, (HBRUSH)GetStockObject(GRAY_BRUSH));
			}
			SetTextColor(hDC, RGB(0xFF, 0xFF, 0xFF));
		}
		else
		{
			SetTextColor(hDC, RGB(0x00, 0x00, 0x00));
		}

		{
			TCHAR sTemp[256];
			_sntprintf(sTemp, countof(sTemp), _T("%0.8X"), nAddress);
			TextOut(hDC, 20, nY, sTemp, (int)_tcslen(sTemp));
		}
		
		CMIPSAnalysis::SUBROUTINE* pSub = m_pCtx->m_pAnalysis->FindSubroutine(nAddress);
		if(pSub != NULL)
		{
			SelectObject(hDC, nLtGrayPen);
			if(nAddress == pSub->nStart)
			{
				MoveToEx(hDC, 90, nY + s.cy + YSPACE, NULL);
				LineTo(hDC, 90, nY + ((s.cy + YSPACE) / 2) - 1);
				LineTo(hDC, 95, nY + ((s.cy + YSPACE) / 2));
			}
			else if(nAddress == pSub->nEnd)
			{
				MoveToEx(hDC, 90, nY, NULL);
				LineTo(hDC, 90, nY + ((s.cy + YSPACE) / 2));
				LineTo(hDC, 95, nY + ((s.cy + YSPACE) / 2));
			}
			else
			{
				MoveToEx(hDC, 90, nY, NULL);
				LineTo(hDC, 90, nY + s.cy + YSPACE);
			}
		}

		uint32 nData = GetInstruction(nAddress);
		DeviceContext.TextOut(100, nY, lexical_cast_hex<std::tstring>(nData, 8).c_str());
		
		{
			char sDisAsm[256];
			m_pCtx->m_pArch->GetInstructionMnemonic(m_pCtx, nAddress, nData, sDisAsm, 256);
			DeviceContext.TextOut(200, nY, string_cast<std::tstring>(sDisAsm).c_str());
		}

		{
			char sDisAsm[256];
			m_pCtx->m_pArch->GetInstructionOperands(m_pCtx, nAddress, nData, sDisAsm, 256);
			DeviceContext.TextOut(300, nY, string_cast<std::tstring>(sDisAsm).c_str());
		}

		bool nCommentDrawn = false;

		{
			const char* sTag = m_pCtx->m_Functions.Find(nAddress);
			if(sTag != NULL)
			{
				char sDisAsm[256];
				SetTextColor(hDC, RGB(0x00, 0x00, 0x80));
				strcpy(sDisAsm, "@");
				strcat(sDisAsm, sTag);
				DeviceContext.TextOut(450, nY, string_cast<std::tstring>(sDisAsm).c_str());
				nCommentDrawn = true;
			}
		}


		if(!nCommentDrawn)
		{
			if(m_pCtx->m_pArch->IsInstructionBranch(m_pCtx, nAddress, nData) == MIPS_BRANCH_NORMAL)
			{
				uint32 nEffAddr = m_pCtx->m_pArch->GetInstructionEffectiveAddress(m_pCtx, nAddress, nData);
				const char* sTag = m_pCtx->m_Functions.Find(nEffAddr);
				if(sTag != NULL)
				{
					char sDisAsm[256];
					SetTextColor(hDC, RGB(0x00, 0x00, 0x80));
					strcpy(sDisAsm, "-> ");
					strcat(sDisAsm, sTag);
					DeviceContext.TextOut(450, nY, string_cast<std::tstring>(sDisAsm).c_str());
					nCommentDrawn = true;
				}
			}
		}

		{
			const char* sTag = m_pCtx->m_Comments.Find(nAddress);
			if(sTag != NULL && !nCommentDrawn)
			{
				char sDisAsm[256];
				SetTextColor(hDC, RGB(0x00, 0x80, 0x00));
				strcpy(sDisAsm, ";");
				strcat(sDisAsm, sTag);
				DeviceContext.TextOut(450, nY, string_cast<std::tstring>(sDisAsm).c_str());
			}
		}

		nY += s.cy + YSPACE;
	}

	DeleteObject(nLtGrayPen);
}
