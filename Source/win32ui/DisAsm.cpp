#include <stdio.h>
#include <boost/bind.hpp>
#include "DisAsm.h"
#include "resource.h"
#include "win32/InputBox.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"
#include "WinUtils.h"
#include "../Ps2Const.h"
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

CDisAsm::CDisAsm(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* ctx)
: m_virtualMachine(virtualMachine)
, m_font(CreateFont(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, _T("Courier New")))
, m_ctx(ctx)
, m_selected(0)
, m_selectionEnd(-1)
, m_address(0)
, m_instructionSize(4)
{
	HistoryReset();

	m_arrowBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ARROW));
	m_arrowMaskBitmap = WinUtils::CreateMask(m_arrowBitmap, 0xFF00FF);
	
	m_breakpointBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BREAKPOINT));
	m_breakpointMaskBitmap = WinUtils::CreateMask(m_breakpointBitmap, 0xFF00FF);

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

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_VISIBLE | WS_VSCROLL | WS_CHILD, rect, hParent, NULL);
	SetClassPtr();

	m_virtualMachine.OnMachineStateChange.connect(boost::bind(&CDisAsm::OnMachineStateChange, this));
	m_virtualMachine.OnRunningStateChange.connect(boost::bind(&CDisAsm::OnRunningStateChange, this));

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.nMin = 0;
	si.nMax = 0x4000;
	si.nPos = 0x2000;
	si.fMask = SIF_RANGE | SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, FALSE);
}

CDisAsm::~CDisAsm()
{
	DeleteObject(m_arrowBitmap);
	DeleteObject(m_arrowMaskBitmap);
	DeleteObject(m_breakpointBitmap);
	DeleteObject(m_breakpointMaskBitmap);
}

void CDisAsm::SetAddress(uint32 address)
{
	m_address = address;
	Redraw();
}

void CDisAsm::SetCenterAtAddress(uint32 address)
{
	unsigned int lineCount = GetLineCount();
	m_address = address - (lineCount * m_instructionSize / 2);
	m_address &= ~(m_instructionSize - 1);
	Redraw();
}

void CDisAsm::SetSelectedAddress(uint32 address)
{
	m_selectionEnd = -1;
	m_selected = address;
	Redraw();
}

void CDisAsm::OnMachineStateChange()
{
	if(!IsAddressVisible(m_ctx->m_State.nPC))
	{
		m_address = m_ctx->m_State.nPC & ~(m_instructionSize - 1);
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
		(_T("0x") + lexical_cast_hex<std::tstring>(m_address, 8)).c_str());

	const TCHAR* sValue = i.GetValue(m_hWnd);
	if(sValue != NULL)
	{
		try
		{
			uint32 nAddress = CDebugExpressionEvaluator::Evaluate(string_cast<std::string>(sValue).c_str(), m_ctx);
			if(nAddress & (m_instructionSize - 1))
			{
				MessageBox(m_hWnd, _T("Invalid address"), NULL, 16);
				return;
			}

			if(m_address != nAddress)
			{
				HistorySave(m_address);
			}

			m_address = nAddress;
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

	m_address = m_ctx->m_State.nPC;
	Redraw();
}

void CDisAsm::GotoEA()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}
	uint32 nOpcode = GetInstruction(m_selected);
	if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, m_selected, nOpcode) == MIPS_BRANCH_NORMAL)
	{
		uint32 nAddress = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, m_selected, nOpcode);

		if(m_address != nAddress)
		{
			HistorySave(m_address);
		}

		m_address = nAddress;
		Redraw();
	}
}

void CDisAsm::EditComment()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}

	const char* comment = m_ctx->m_Comments.Find(m_selected);
	std::tstring commentConv;

	if(comment != nullptr)
	{
		commentConv = string_cast<std::tstring>(comment);
	}
	else
	{
		commentConv = _T("");
	}

	Framework::Win32::CInputBox i(_T("Edit Comment"), _T("Enter new comment:"), commentConv.c_str());
	const TCHAR* value = i.GetValue(m_hWnd);

	if(value != nullptr)
	{
		m_ctx->m_Comments.InsertTag(m_selected, string_cast<std::string>(value).c_str());
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

	for(int i = 0; i < PS2::EE_RAM_SIZE; i += 4)
	{
		uint32 nVal = m_ctx->m_pMemoryMap->GetInstruction(i);
		if(((nVal & 0xFC000000) == 0x0C000000) || ((nVal & 0xFC000000) == 0x08000000))
		{
			nVal &= 0x3FFFFFF;
			nVal *= 4;
			if(nVal == m_selected)
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
	RECT rwin = GetClientRect();
	unsigned int nFontCY = GetFontHeight();
	unsigned int nLines = (rwin.bottom - (YMARGIN * 2)) / (nFontCY + YSPACE);
	nLines++;

	return nLines;
}

bool CDisAsm::IsAddressVisible(uint32 nAddress)
{
	uint32 nTop = m_address;
	uint32 nBottom = nTop + ((GetLineCount() - 1) * m_instructionSize);

	if(nAddress < nTop) return false;
	if(nAddress > nBottom) return false;

	return true;
}

CDisAsm::SelectionRangeType CDisAsm::GetSelectionRange()
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

void CDisAsm::HistoryReset()
{
	m_historyPosition	= -1;
	m_historySize		= 0;
	memset(m_history, 0, sizeof(uint32) * HISTORY_STACK_MAX);
}

void CDisAsm::HistorySave(uint32 nAddress)
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

void CDisAsm::HistoryGoBack()
{
	if(m_historyPosition == -1) return;

	uint32 address = HistoryGetPrevious();
	m_history[m_historyPosition] = m_address;
	m_address = address;

	m_historyPosition--;
	Redraw();
}

void CDisAsm::HistoryGoForward()
{
	if(m_historyPosition == m_historySize) return;

	uint32 address = HistoryGetNext();
	m_historyPosition++;
	m_history[m_historyPosition] = m_address;
	m_address = address;

	Redraw();
}

uint32 CDisAsm::HistoryGetPrevious()
{
	return m_history[m_historyPosition];
}

uint32 CDisAsm::HistoryGetNext()
{
	if(m_historyPosition == m_historySize) return 0;
	return m_history[m_historyPosition + 1];
}

bool CDisAsm::HistoryHasPrevious()
{
	return (m_historySize != 0) && (m_historyPosition != -1);
}

bool CDisAsm::HistoryHasNext()
{
	return (m_historySize != 0) && (m_historyPosition != (m_historySize - 1));
}

void CDisAsm::UpdateMouseSelection(unsigned int nX, unsigned int nY)
{
	if(nX <= 18) return;

	uint32 nNew = nY / (GetFontHeight() + YSPACE);
	nNew = (m_address + (nNew * m_instructionSize));

	if(GetKeyState(VK_SHIFT) & 0x8000)
	{
		m_selectionEnd = nNew;
	}
	else
	{
		m_selectionEnd = -1;
		if(nNew == m_selected) return;
		m_selected = nNew;
	}

	Redraw();
}

uint32 CDisAsm::GetAddressAtPosition(unsigned int nX, unsigned int nY)
{
	uint32 address = nY / (GetFontHeight() + YSPACE);
	address = (m_address + (address * m_instructionSize));
	return address;
}

uint32 CDisAsm::GetInstruction(uint32 address)
{
	//Address translation perhaps?
	return m_ctx->m_pMemoryMap->GetInstruction(address);
}

void CDisAsm::ToggleBreakpoint(uint32 address)
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}
	m_ctx->ToggleBreakpoint(address);
	Redraw();
}

void CDisAsm::UpdatePosition(int delta)
{
	m_address += delta;
	Redraw();
}

long CDisAsm::OnSetFocus()
{
	if(m_focus) return TRUE;
	m_focus = true;
	Redraw();
	return TRUE;
}

long CDisAsm::OnKillFocus()
{
	m_focus = false;
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

	if(m_selected != MIPS_INVALID_PC)
	{
		uint32 nOpcode = GetInstruction(m_selected);
		if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, m_selected, nOpcode) == MIPS_BRANCH_NORMAL)
		{
			TCHAR sTemp[256];
			uint32 nAddress = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, m_selected, nOpcode);
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
	if(m_focus)
	{
		UpdateMouseSelection(nX, nY);
	}
	return FALSE;
}

long CDisAsm::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	CCustomDrawn::OnSize(nType, nX, nY);
	return TRUE;
}

long CDisAsm::OnMouseWheel(int x, int y, short z)
{
	if(z < 0)
	{
		UpdatePosition(m_instructionSize * 3);
	}
	else
	{
		UpdatePosition(-m_instructionSize * 3);
	}
	return FALSE;
}

long CDisAsm::OnVScroll(unsigned int nType, unsigned int nTrackPos)
{
	switch(nType)
	{
	case SB_LINEDOWN:
		UpdatePosition(m_instructionSize);
		break;
	case SB_LINEUP:
		UpdatePosition(-m_instructionSize);
		break;
	case SB_PAGEDOWN:
		UpdatePosition(m_instructionSize * 10);
		break;
	case SB_PAGEUP:
		UpdatePosition(-m_instructionSize * 10);
		break;
	}
	return FALSE;
}

long CDisAsm::OnKeyDown(unsigned int nKey)
{
	switch(nKey)
	{
	case 'C':
		if(GetAsyncKeyState(VK_CONTROL))
		{
			SendMessage(m_hWnd, WM_COPY, 0, 0);
		}
		break;
	case VK_F9:
		ToggleBreakpoint(m_selected);
		break;
	case VK_DOWN:
		m_selected += m_instructionSize;
		if(!IsAddressVisible(m_selected))
		{
			UpdatePosition(m_instructionSize);
		}
		else
		{
			Redraw();
		}
		break;
	case VK_UP:
		m_selected -= m_instructionSize;
		if(!IsAddressVisible(m_selected))
		{
			UpdatePosition(-m_instructionSize);
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

	std::tstring text;

	auto selectionRange = GetSelectionRange();
	for(uint32 address = selectionRange.first; address <= selectionRange.second; address += m_instructionSize)
	{
		if(address != selectionRange.first)
		{
			text += _T("\r\n");
		}
		text += GetInstructionDetailsText(address);
	}

	HGLOBAL hMemory = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(TCHAR));
	_tcscpy(reinterpret_cast<TCHAR*>(GlobalLock(hMemory)), text.c_str());
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
	Framework::Win32::CDeviceContext deviceContext(hDC);

	RECT rwin = GetClientRect();

	BitBlt(hDC, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

	SIZE s;
	deviceContext.SelectObject(m_font);
	GetTextExtentPoint32(hDC, _T("0"), 1, &s);

	int lines = (rwin.bottom - (YMARGIN * 2)) / (s.cy + YSPACE);
	lines++;

	RECT rmarg;
	SetRect(&rmarg, 0, 0, 17, rwin.bottom);
	FillRect(hDC, &rmarg, (HBRUSH)COLOR_WINDOW);

	HPEN ltGrayPen = CreatePen(PS_SOLID, 2, RGB(0x40, 0x40, 0x40));

	//Draw the margin border line
	{
		HPEN pen = CreatePen(PS_SOLID, 0, RGB(0x80, 0x80, 0x80));
		SelectObject(hDC, pen);
		MoveToEx(hDC, 17, 0, NULL);
		LineTo(hDC, 17, rwin.bottom);
		DeleteObject(pen);
	}

	SetBkMode(hDC, TRANSPARENT);

	unsigned int y = YMARGIN;

	SelectionRangeType SelectionRange = GetSelectionRange();

	for(int i = 0; i < lines; i++)
	{
		uint32 address = m_address + (i * m_instructionSize);
		
		//Draw breakpoint icon
		if(m_ctx->m_breakpoints.find(address) != std::end(m_ctx->m_breakpoints))
		{
			SetTextColor(hDC, RGB(0x00, 0x00, 0x00));

			{
				HDC hMem = CreateCompatibleDC(hDC);
				SelectObject(hMem, m_breakpointMaskBitmap);
				BitBlt(hDC, 1, y + 1, 15, 15, hMem, 0, 0, SRCAND);
				DeleteDC(hMem);
			}

			{
				HDC hMem = CreateCompatibleDC(hDC);
				SelectObject(hMem, m_breakpointBitmap);
				BitBlt(hDC, 1, y + 1, 15, 15, hMem, 0, 0, SRCPAINT);
				DeleteDC(hMem);
			}
		}

		//Draw current instruction arrow
		if(m_virtualMachine.GetStatus() != CVirtualMachine::RUNNING)
		{
			if(address == m_ctx->m_State.nPC)
			{
				SetTextColor(hDC, RGB(0x00, 0x00, 0x00));

				{
					HDC hMem = CreateCompatibleDC(hDC);
					SelectObject(hMem, m_arrowMaskBitmap);
					BitBlt(hDC, 3, y + 2, 13, 13, hMem, 0, 0, SRCAND);
					DeleteDC(hMem);
				}

				{
					HDC hMem = CreateCompatibleDC(hDC);
					SelectObject(hMem, m_arrowBitmap);
					BitBlt(hDC, 3, y + 2, 13, 13, hMem, 0, 0, SRCPAINT);
					DeleteDC(hMem);
				}
			}
		}

		if(
			(address >= SelectionRange.first) && 
			(address <= SelectionRange.second)
			)
		{
			RECT rsel;
			SetRect(&rsel, 18, y, rwin.right, y + s.cy + YSPACE);
			if(m_focus)
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

		//Draw address
		{
			TCHAR sTemp[256];
			_sntprintf(sTemp, countof(sTemp), _T("%0.8X"), address);
			TextOut(hDC, 20, y, sTemp, (int)_tcslen(sTemp));
		}
		
		//Draw function boundaries
		const CMIPSAnalysis::SUBROUTINE* sub = m_ctx->m_pAnalysis->FindSubroutine(address);
		if(sub != NULL)
		{
			SelectObject(hDC, ltGrayPen);
			if(address == sub->nStart)
			{
				MoveToEx(hDC, 90, y + s.cy + YSPACE, NULL);
				LineTo(hDC, 90, y + ((s.cy + YSPACE) / 2) - 1);
				LineTo(hDC, 95, y + ((s.cy + YSPACE) / 2));
			}
			else if(address == sub->nEnd)
			{
				MoveToEx(hDC, 90, y, NULL);
				LineTo(hDC, 90, y + ((s.cy + YSPACE) / 2));
				LineTo(hDC, 95, y + ((s.cy + YSPACE) / 2));
			}
			else
			{
				MoveToEx(hDC, 90, y, NULL);
				LineTo(hDC, 90, y + s.cy + YSPACE);
			}
		}

		DrawInstructionDetails(deviceContext, address, y);
		DrawInstructionMetadata(deviceContext, address, y);

		y += s.cy + YSPACE;
	}

	DeleteObject(ltGrayPen);
}

std::tstring CDisAsm::GetInstructionDetailsText(uint32 address)
{
	uint32 opcode = GetInstruction(address);

	std::tstring result;

	result += lexical_cast_hex<std::tstring>(address, 8) + _T("    ");
	result += lexical_cast_hex<std::tstring>(opcode,  8) + _T("    ");

	char disasm[256];
	m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address, opcode, disasm, countof(disasm));

	result += string_cast<std::tstring>(disasm);
	for(size_t j = strlen(disasm); j < 15; j++)
	{
		result += _T(" ");
	}

	m_ctx->m_pArch->GetInstructionOperands(m_ctx, address, opcode, disasm, countof(disasm));
	result += string_cast<std::tstring>(disasm);

	return result;
}

void CDisAsm::DrawInstructionDetails(Framework::Win32::CDeviceContext& deviceContext, uint32 address, int y)
{
	uint32 data = GetInstruction(address);
	deviceContext.TextOut(100, y, lexical_cast_hex<std::tstring>(data, 8).c_str());
		
	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address, data, disAsm, 256);
		deviceContext.TextOut(200, y, string_cast<std::tstring>(disAsm).c_str());
	}

	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address, data, disAsm, 256);
		deviceContext.TextOut(300, y, string_cast<std::tstring>(disAsm).c_str());
	}
}

void CDisAsm::DrawInstructionMetadata(Framework::Win32::CDeviceContext& deviceContext, uint32 address, int y)
{
	bool commentDrawn = false;

	//Draw function name
	{
		const char* tag = m_ctx->m_Functions.Find(address);
		if(tag != nullptr)
		{
			std::tstring disAsm = _T("@") + string_cast<std::tstring>(tag);
			deviceContext.TextOut(450, y, disAsm.c_str(), RGB(0x00, 0x00, 0x80));
			commentDrawn = true;
		}
	}

	//Draw target function call if applicable
	if(!commentDrawn)
	{
		uint32 opcode = GetInstruction(address);
		if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, address, opcode) == MIPS_BRANCH_NORMAL)
		{
			uint32 effAddr = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, address, opcode);
			const char* tag = m_ctx->m_Functions.Find(effAddr);
			if(tag != nullptr)
			{
				std::tstring disAsm = _T("-> ") + string_cast<std::tstring>(tag);
				deviceContext.TextOut(450, y, disAsm.c_str(), RGB(0x00, 0x00, 0x80));
				commentDrawn = true;
			}
		}
	}

	//Draw comment
	if(!commentDrawn)
	{
		const char* tag = m_ctx->m_Comments.Find(address);
		if(tag != nullptr)
		{
			std::tstring disAsm = _T(";") + string_cast<std::tstring>(tag);
			deviceContext.TextOut(450, y, disAsm.c_str(), RGB(0x00, 0x80, 0x00));
			commentDrawn = true;
		}
	}
}
