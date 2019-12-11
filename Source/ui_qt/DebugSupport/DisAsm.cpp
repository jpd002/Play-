#include <QString>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>


#include <stdio.h>
#include "countof.h"
#include "DisAsm.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"
#include "Ps2Const.h"
#include "DebugExpressionEvaluator.h"

#define ID_DISASM_GOTOPC 40001
#define ID_DISASM_GOTOADDRESS 40002
#define ID_DISASM_GOTOEA 40003
#define ID_DISASM_EDITCOMMENT 40004
#define ID_DISASM_FINDCALLERS 40005
#define ID_DISASM_GOTOPREV 40006
#define ID_DISASM_GOTONEXT 40007

CDisAsm::CDisAsm(QMdiSubWindow* parent, CVirtualMachine& virtualMachine, CMIPS* ctx)
    : m_parent(parent)
    , m_virtualMachine(virtualMachine)
    // , m_font(Framework::Win32::CreateFont(("Courier New"), 8))
    , m_ctx(ctx)
    , m_selected(0)
    , m_selectionEnd(-1)
    , m_address(0)
    , m_instructionSize(4)
{
	HistoryReset();

	m_tableView = new QTableView(m_parent);
	m_parent->setWidget(m_tableView);
	m_model = new CQtDisAsmTableModel(parent, virtualMachine, m_ctx, {"S", "Address", "R", "Instr","I-Mn", "I-Op", "Name"});
	m_tableView->setModel(m_model);
	auto header = m_tableView->horizontalHeader();
	header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(6, QHeaderView::Stretch);
	// m_tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_tableView->verticalHeader()->hide();

	// m_arrowBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ARROW));
	// m_arrowMaskBitmap = WinUtils::CreateMask(m_arrowBitmap, 0xFF00FF);

	// m_breakpointBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BREAKPOINT));
	// m_breakpointMaskBitmap = WinUtils::CreateMask(m_breakpointBitmap, 0xFF00FF);
}

CDisAsm::~CDisAsm()
{
}

void CDisAsm::SetAddress(uint32 address)
{
	m_tableView->scrollTo(m_model->index(address / m_instructionSize, 0), QAbstractItemView::PositionAtTop);
	m_address = address;
}

void CDisAsm::SetCenterAtAddress(uint32 address)
{
	m_tableView->scrollTo(m_model->index(m_address / m_instructionSize, 0), QAbstractItemView::PositionAtCenter);
}

void CDisAsm::SetSelectedAddress(uint32 address)
{
	m_selectionEnd = -1;
	m_selected = address;
	auto index = m_model->index(address / m_instructionSize, 0);
	m_tableView->scrollTo(index, QAbstractItemView::PositionAtTop);
	m_tableView->setCurrentIndex(index);
}

void CDisAsm::HandleMachineStateChange()
{
	m_model->Redraw();
}

void CDisAsm::HandleRunningStateChange(CVirtualMachine::STATUS newState)
{
	if(newState == CVirtualMachine::STATUS::PAUSED)
	{
		//Recenter view if we just got into paused state
		m_address = m_ctx->m_State.nPC & ~(m_instructionSize - 1);
		SetCenterAtAddress(m_address);
	}
	m_model->Redraw();
}

void CDisAsm::GotoAddress()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		// MessageBeep(-1);
		return;
	}

	bool ok;
	QString sValue = QInputDialog::getText(m_parent, ("Goto Address"),
										("Enter new address:"), QLineEdit::Normal,
										((("0x") + lexical_cast_hex<std::string>(m_address, 8)).c_str()), &ok);
	if (!ok  || sValue.isEmpty())
		return;

	{
		try
		{
			uint32 nAddress = CDebugExpressionEvaluator::Evaluate(sValue.toStdString().c_str(), m_ctx);
			if(nAddress & (m_instructionSize - 1))
			{
				// MessageBox(m_hWnd, ("Invalid address"), NULL, 16);
				return;
			}

			if(m_address != nAddress)
			{
				HistorySave(m_address);
			}

			m_address = nAddress;
			SetAddress(nAddress);
		}
		catch(const std::exception& exception)
		{
			std::string message = std::string("Error evaluating expression: ") + exception.what();
			// MessageBox(m_hWnd, message.c_str(), NULL, 16);
		}
	}
}

void CDisAsm::GotoPC()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		// MessageBeep(-1);
		return;
	}

	m_address = m_ctx->m_State.nPC;
	SetAddress(m_ctx->m_State.nPC);
}

void CDisAsm::GotoEA()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		// MessageBeep(-1);
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
		SetAddress(nAddress);
	}
}

void CDisAsm::EditComment()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		// MessageBeep(-1);
		return;
	}

	const char* comment = m_ctx->m_Comments.Find(m_selected);
	std::string commentConv;

	if(comment != nullptr)
	{
		commentConv = comment;
	}
	else
	{
		commentConv = ("");
	}

	bool ok;
	QString value = QInputDialog::getText(m_parent, ("Edit Comment"),
										("Enter new comment:"), QLineEdit::Normal,
										(commentConv.c_str()), &ok);
	if (!ok  || value.isEmpty())
		return;

	// if(value != nullptr)
	{
		m_ctx->m_Comments.InsertTag(m_selected, value.toStdString().c_str());
		m_model->Redraw(m_selected);
	}
}

void CDisAsm::FindCallers()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		// MessageBeep(-1);
		return;
	}

	FindCallersRequested(m_selected);
}

unsigned int CDisAsm::GetLineCount()
{
	// auto clientRect = GetClientRect();
	// unsigned int lineStep = (m_renderMetrics.fontSizeY + m_renderMetrics.yspace);
	// unsigned int lines = (clientRect.Bottom() - (m_renderMetrics.ymargin * 2)) / lineStep;
	// lines++;
	// return lines;
	return 15;
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
	m_historyPosition = -1;
	m_historySize = 0;
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
	SetAddress(address);

}

void CDisAsm::HistoryGoForward()
{
	if(m_historyPosition == m_historySize) return;

	uint32 address = HistoryGetNext();
	m_historyPosition++;
	m_history[m_historyPosition] = m_address;
	m_address = address;
	SetAddress(address);
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

// void CDisAsm::UpdateMouseSelection(unsigned int nX, unsigned int nY)
// {
// 	if(nX <= m_renderMetrics.xmargin) return;

// 	uint32 nNew = nY / (m_renderMetrics.fontSizeY + m_renderMetrics.yspace);
// 	nNew = (m_address + (nNew * m_instructionSize));

// 	if(GetKeyState(VK_SHIFT) & 0x8000)
// 	{
// 		m_selectionEnd = nNew;
// 	}
// 	else
// 	{
// 		m_selectionEnd = -1;
// 		if(nNew == m_selected) return;
// 		m_selected = nNew;
// 	}

// 	// Redraw();
// }

uint32 CDisAsm::GetAddressAtPosition(unsigned int nX, unsigned int nY)
{
	uint32 address = nY / (m_renderMetrics.fontSizeY + m_renderMetrics.yspace);
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
		// MessageBeep(-1);
		return;
	}
	m_ctx->ToggleBreakpoint(address);
	m_model->Redraw(address);
}

void CDisAsm::UpdatePosition(int delta)
{
	m_address += delta;
	SetAddress(m_address);
}

// long CDisAsm::OnSetFocus()
// {
// 	if(m_focus) return TRUE;
// 	m_focus = true;
// 	// Redraw();
// 	return TRUE;
// }

// long CDisAsm::OnKillFocus()
// {
// 	m_focus = false;
// 	// Redraw();
// 	return TRUE;
// }

// long CDisAsm::OnLeftButtonDown(int nX, int nY)
// {
// 	UpdateMouseSelection(nX, nY);
// 	SetFocus();
// 	return FALSE;
// }

// long CDisAsm::OnRightButtonUp(int nX, int nY)
// {
// 	POINT pt;
// 	pt.x = nX;
// 	pt.y = nY;
// 	ClientToScreen(m_hWnd, &pt);

// 	HMENU hMenu = CreatePopupMenu();
// 	unsigned int position = BuildContextMenu(hMenu);

// 	if(m_selected != MIPS_INVALID_PC)
// 	{
// 		uint32 nOpcode = GetInstruction(m_selected);
// 		if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, m_selected, nOpcode) == MIPS_BRANCH_NORMAL)
// 		{
// 			TCHAR sTemp[256];
// 			uint32 nAddress = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, m_selected, nOpcode);
// 			_sntprintf(sTemp, countof(sTemp), ("Go to 0x%08X"), nAddress);
// 			InsertMenu(hMenu, position++, MF_BYPOSITION, ID_DISASM_GOTOEA, sTemp);
// 		}
// 	}

// 	if(HistoryHasPrevious())
// 	{
// 		TCHAR sTemp[256];
// 		_sntprintf(sTemp, countof(sTemp), ("Go back (0x%08X)"), HistoryGetPrevious());
// 		InsertMenu(hMenu, position++, MF_BYPOSITION, ID_DISASM_GOTOPREV, sTemp);
// 	}

// 	if(HistoryHasNext())
// 	{
// 		TCHAR sTemp[256];
// 		_sntprintf(sTemp, countof(sTemp), ("Go forward (0x%08X)"), HistoryGetNext());
// 		InsertMenu(hMenu, position++, MF_BYPOSITION, ID_DISASM_GOTONEXT, sTemp);
// 	}

// 	TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_hWnd, NULL);

// 	return FALSE;
// }

// long CDisAsm::OnMouseMove(WPARAM nButton, int nX, int nY)
// {
// 	if(!(nButton & MK_LBUTTON)) return TRUE;
// 	if(m_focus)
// 	{
// 		UpdateMouseSelection(nX, nY);
// 	}
// 	return FALSE;
// }

// long CDisAsm::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
// {
// 	CCustomDrawn::OnSize(nType, nX, nY);
// 	return TRUE;
// }

// long CDisAsm::OnMouseWheel(int x, int y, short z)
// {
// 	if(z < 0)
// 	{
// 		UpdatePosition(m_instructionSize * 3);
// 	}
// 	else
// 	{
// 		UpdatePosition(-m_instructionSize * 3);
// 	}
// 	return FALSE;
// }

// long CDisAsm::OnVScroll(unsigned int nType, unsigned int nTrackPos)
// {
// 	switch(nType)
// 	{
// 	case SB_LINEDOWN:
// 		UpdatePosition(m_instructionSize);
// 		break;
// 	case SB_LINEUP:
// 		UpdatePosition(-m_instructionSize);
// 		break;
// 	case SB_PAGEDOWN:
// 		UpdatePosition(m_instructionSize * 10);
// 		break;
// 	case SB_PAGEUP:
// 		UpdatePosition(-m_instructionSize * 10);
// 		break;
// 	}
// 	return FALSE;
// }

// long CDisAsm::OnKeyDown(WPARAM nKey, LPARAM)
// {
// 	switch(nKey)
// 	{
// 	case 'C':
// 		if(GetAsyncKeyState(VK_CONTROL))
// 		{
// 			SendMessage(m_hWnd, WM_COPY, 0, 0);
// 		}
// 		break;
// 	case VK_F9:
// 		ToggleBreakpoint(m_selected);
// 		break;
// 	case VK_DOWN:
// 		m_selected += m_instructionSize;
// 		if(!IsAddressVisible(m_selected))
// 		{
// 			UpdatePosition(m_instructionSize);
// 		}
// 		else
// 		{
// 			// Redraw();
// 		}
// 		break;
// 	case VK_UP:
// 		m_selected -= m_instructionSize;
// 		if(!IsAddressVisible(m_selected))
// 		{
// 			UpdatePosition(-m_instructionSize);
// 		}
// 		else
// 		{
// 			// Redraw();
// 		}
// 		break;
// 	case VK_RIGHT:
// 		if(m_selected != MIPS_INVALID_PC)
// 		{
// 			uint32 nOpcode = GetInstruction(m_selected);
// 			if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, m_selected, nOpcode) == MIPS_BRANCH_NORMAL)
// 			{
// 				m_address = m_selected; // Ensure history tracks where we came from
// 				GotoEA();
// 				SetSelectedAddress(m_address);
// 			}
// 			else if(HistoryHasNext())
// 			{
// 				HistoryGoForward();
// 				SetSelectedAddress(m_address);
// 			}
// 		}
// 		break;
// 	case VK_LEFT:
// 		if(HistoryHasPrevious())
// 		{
// 			HistoryGoBack();
// 			SetSelectedAddress(m_address);
// 		}
// 		break;
// 	}
// 	return TRUE;
// }

// long CDisAsm::OnLeftButtonDblClk(int nX, int nY)
// {
// 	if(nX > m_renderMetrics.xmargin)
// 	{
// 	}
// 	else
// 	{
// 		ToggleBreakpoint(GetAddressAtPosition(nX, nY));
// 	}
// 	return FALSE;
// }

// long CDisAsm::OnCommand(unsigned short nID, unsigned short nMsg, HWND hFrom)
// {
// 	switch(nID)
// 	{
// 	case ID_DISASM_GOTOPC:
// 		GotoPC();
// 		return FALSE;
// 		break;
// 	case ID_DISASM_GOTOADDRESS:
// 		GotoAddress();
// 		return FALSE;
// 		break;
// 	case ID_DISASM_GOTOEA:
// 		GotoEA();
// 		return FALSE;
// 		break;
// 	case ID_DISASM_EDITCOMMENT:
// 		EditComment();
// 		return FALSE;
// 		break;
// 	case ID_DISASM_FINDCALLERS:
// 		FindCallers();
// 		return FALSE;
// 		break;
// 	case ID_DISASM_GOTOPREV:
// 		HistoryGoBack();
// 		break;
// 	case ID_DISASM_GOTONEXT:
// 		HistoryGoForward();
// 		break;
// 	}
// 	return TRUE;
// }

// long CDisAsm::OnCopy()
// {
// 	std::string text;

// 	auto selectionRange = GetSelectionRange();
// 	for(uint32 address = selectionRange.first; address <= selectionRange.second; address += m_instructionSize)
// 	{
// 		if(address != selectionRange.first)
// 		{
// 			text += ("\r\n");
// 		}
// 		text += GetInstructionDetailsText(address);
// 	}

// 	WinUtils::CopyStringToClipboard(text);

// 	return TRUE;
// }

// static void DrawMaskedBitmap(HDC dstDc, Framework::Win32::CRect& dstRect,
//                              HBITMAP srcBitmap, HBITMAP srcMaskBitmap, Framework::Win32::CRect& srcRect)
// {
// 	SetTextColor(dstDc, RGB(0x00, 0x00, 0x00));

// 	{
// 		HDC memDc = CreateCompatibleDC(dstDc);
// 		SelectObject(memDc, srcMaskBitmap);
// 		StretchBlt(
// 		    dstDc, dstRect.Left(), dstRect.Top(), dstRect.Width(), dstRect.Height(),
// 		    memDc, srcRect.Left(), srcRect.Top(), srcRect.Width(), srcRect.Height(), SRCAND);
// 		DeleteDC(memDc);
// 	}

// 	{
// 		HDC memDc = CreateCompatibleDC(dstDc);
// 		SelectObject(memDc, srcBitmap);
// 		StretchBlt(
// 		    dstDc, dstRect.Left(), dstRect.Top(), dstRect.Width(), dstRect.Height(),
// 		    memDc, srcRect.Left(), srcRect.Top(), srcRect.Width(), srcRect.Height(), SRCPAINT);
// 		DeleteDC(memDc);
// 	}
// }

// void CDisAsm::Paint(HDC hDC)
// {
// 	Framework::Win32::CDeviceContext deviceContext(hDC);

// 	RECT rwin = GetClientRect();

// 	BitBlt(hDC, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

// 	deviceContext.SelectObject(m_font);

// 	int lineStep = (m_renderMetrics.fontSizeY + m_renderMetrics.yspace);
// 	int textOffset = (lineStep - m_renderMetrics.fontSizeY) / 2;

// 	int lines = (rwin.bottom - (m_renderMetrics.ymargin * 2)) / lineStep;
// 	lines++;

// 	RECT rmarg;
// 	SetRect(&rmarg, 0, 0, m_renderMetrics.leftBarSize, rwin.bottom);
// 	FillRect(hDC, &rmarg, (HBRUSH)COLOR_WINDOW);

// 	Framework::Win32::CPen ltGrayPen = CreatePen(PS_SOLID, Framework::Win32::PointsToPixels(2), RGB(0x40, 0x40, 0x40));

// 	//Draw the margin border line
// 	{
// 		Framework::Win32::CPen pen = CreatePen(PS_SOLID, 0, RGB(0x80, 0x80, 0x80));
// 		SelectObject(hDC, pen);
// 		MoveToEx(hDC, m_renderMetrics.leftBarSize, 0, NULL);
// 		LineTo(hDC, m_renderMetrics.leftBarSize, rwin.bottom);
// 	}

// 	SetBkMode(hDC, TRANSPARENT);

// 	unsigned int y = m_renderMetrics.ymargin;

// 	SelectionRangeType SelectionRange = GetSelectionRange();

// 	for(int i = 0; i < lines; i++)
// 	{
// 		uint32 address = m_address + (i * m_instructionSize);

// 		auto iconArea = Framework::Win32::CRect(0, y, m_renderMetrics.xmargin, y + lineStep);

// 		//Draw breakpoint icon
// 		if(m_ctx->m_breakpoints.find(address) != std::end(m_ctx->m_breakpoints))
// 		{
// 			auto breakpointSrcRect = Framework::Win32::MakeRectPositionSize(0, 0, 15, 15);
// 			auto breakpointDstRect = Framework::Win32::PointsToPixels(breakpointSrcRect).CenterInside(iconArea);
// 			DrawMaskedBitmap(hDC, breakpointDstRect, m_breakpointBitmap, m_breakpointMaskBitmap, breakpointSrcRect);
// 		}

// 		//Draw current instruction arrow
// 		if(m_virtualMachine.GetStatus() != CVirtualMachine::RUNNING)
// 		{
// 			if(address == m_ctx->m_State.nPC)
// 			{
// 				auto arrowSrcRect = Framework::Win32::MakeRectPositionSize(0, 0, 13, 13);
// 				auto arrowDstRect = Framework::Win32::PointsToPixels(arrowSrcRect).CenterInside(iconArea);
// 				DrawMaskedBitmap(hDC, arrowDstRect, m_arrowBitmap, m_arrowMaskBitmap, arrowSrcRect);
// 			}
// 		}

// 		bool selected = false;

// 		if(
// 		    (address >= SelectionRange.first) &&
// 		    (address <= SelectionRange.second))
// 		{
// 			RECT rsel;
// 			SetRect(&rsel, m_renderMetrics.xmargin, y, rwin.right, y + lineStep);
// 			if(m_focus)
// 			{
// 				FillRect(hDC, &rsel, (HBRUSH)GetStockObject(BLACK_BRUSH));
// 			}
// 			else
// 			{
// 				FillRect(hDC, &rsel, (HBRUSH)GetStockObject(GRAY_BRUSH));
// 			}
// 			SetTextColor(hDC, RGB(0xFF, 0xFF, 0xFF));
// 			selected = true;
// 		}
// 		else
// 		{
// 			SetTextColor(hDC, RGB(0x00, 0x00, 0x00));
// 		}

// 		//Draw address
// 		deviceContext.TextOut(m_renderMetrics.xtextStart, y + textOffset,
// 		                      string_format(("%08X"), address).c_str());

// 		//Draw function boundaries
// 		const auto* sub = m_ctx->m_analysis->FindSubroutine(address);
// 		if(sub != nullptr)
// 		{
// 			SelectObject(hDC, ltGrayPen);
// 			if(address == sub->start)
// 			{
// 				MoveToEx(hDC, GetFuncBoundaryPosition(), y + lineStep, NULL);
// 				LineTo(hDC, GetFuncBoundaryPosition(), y + (lineStep / 2) - 1);
// 				LineTo(hDC, GetFuncBoundaryPosition() + m_renderMetrics.fontSizeX * 0.5f, y + (lineStep / 2));
// 			}
// 			else if(address == sub->end)
// 			{
// 				MoveToEx(hDC, GetFuncBoundaryPosition(), y, NULL);
// 				LineTo(hDC, GetFuncBoundaryPosition(), y + (lineStep / 2));
// 				LineTo(hDC, GetFuncBoundaryPosition() + m_renderMetrics.fontSizeX * 0.5f, y + (lineStep / 2));
// 			}
// 			else
// 			{
// 				MoveToEx(hDC, GetFuncBoundaryPosition(), y, NULL);
// 				LineTo(hDC, GetFuncBoundaryPosition(), y + lineStep);
// 			}
// 		}

// 		DrawInstructionDetails(deviceContext, address, y + textOffset);
// 		DrawInstructionMetadata(deviceContext, address, y + textOffset, selected);

// 		y += lineStep;
// 	}
// }

// unsigned int CDisAsm::BuildContextMenu(HMENU menuHandle)
// {
// 	unsigned int position = 0;
// 	InsertMenu(menuHandle, position++, MF_BYPOSITION, ID_DISASM_GOTOPC, ("Goto PC"));
// 	InsertMenu(menuHandle, position++, MF_BYPOSITION, ID_DISASM_GOTOADDRESS, ("Goto Address..."));
// 	InsertMenu(menuHandle, position++, MF_BYPOSITION, ID_DISASM_EDITCOMMENT, ("Edit Comment..."));
// 	InsertMenu(menuHandle, position++, MF_BYPOSITION, ID_DISASM_FINDCALLERS, ("Find Callers"));
// 	return position;
// }

std::string CDisAsm::GetInstructionDetailsText(uint32 address)
{
	uint32 opcode = GetInstruction(address);

	std::string result;

	result += lexical_cast_hex<std::string>(address, 8) + ("    ");
	result += lexical_cast_hex<std::string>(opcode, 8) + ("    ");

	char disasm[256];
	m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address, opcode, disasm, countof(disasm));

	result += disasm;
	for(size_t j = strlen(disasm); j < 15; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionOperands(m_ctx, address, opcode, disasm, countof(disasm));
	result += disasm;

	return result;
}

unsigned int CDisAsm::GetMetadataPosition() const
{
	return m_renderMetrics.fontSizeX * 55;
}

unsigned int CDisAsm::GetFuncBoundaryPosition() const
{
	unsigned int addressEnd = m_renderMetrics.xtextStart + (8 * m_renderMetrics.fontSizeX);
	unsigned int instructionStart = 16 * m_renderMetrics.fontSizeX;

	return ((instructionStart + addressEnd) / 2);
}

// void CDisAsm::DrawInstructionDetails(Framework::Win32::CDeviceContext& deviceContext, uint32 address, int y)
// {
// 	uint32 data = GetInstruction(address);
// 	deviceContext.TextOut(m_renderMetrics.fontSizeX * 15, y, lexical_cast_hex<std::string>(data, 8).c_str());

// 	{
// 		char disAsm[256];
// 		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address, data, disAsm, 256);
// 		deviceContext.TextOut(m_renderMetrics.fontSizeX * 25, y, string_cast<std::string>(disAsm).c_str());
// 	}

// 	{
// 		char disAsm[256];
// 		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address, data, disAsm, 256);
// 		deviceContext.TextOut(m_renderMetrics.fontSizeX * 35, y, string_cast<std::string>(disAsm).c_str());
// 	}
// }

// void CDisAsm::DrawInstructionMetadata(Framework::Win32::CDeviceContext& deviceContext, uint32 address, int y, bool selected)
// {
// 	bool commentDrawn = false;
// 	unsigned int metadataPosition = GetMetadataPosition();

// 	//Draw function name
// 	{
// 		const char* tag = m_ctx->m_Functions.Find(address);
// 		if(tag != nullptr)
// 		{
// 			std::string disAsm = ("@") + string_cast<std::string>(tag);
// 			deviceContext.TextOut(metadataPosition, y, disAsm.c_str(), selected ? RGB(0x40, 0x80, 0xFF) : RGB(0x00, 0x00, 0x80));
// 			commentDrawn = true;
// 		}
// 	}

// 	//Draw target function call if applicable
// 	if(!commentDrawn)
// 	{
// 		uint32 opcode = GetInstruction(address);
// 		if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, address, opcode) == MIPS_BRANCH_NORMAL)
// 		{
// 			uint32 effAddr = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, address, opcode);
// 			const char* tag = m_ctx->m_Functions.Find(effAddr);
// 			if(tag != nullptr)
// 			{
// 				std::string disAsm = ("-> ") + string_cast<std::string>(tag);
// 				deviceContext.TextOut(metadataPosition, y, disAsm.c_str(), selected ? RGB(0x40, 0x80, 0xFF) : RGB(0x00, 0x00, 0x80));
// 				commentDrawn = true;
// 			}
// 		}
// 	}

// 	//Draw comment
// 	if(!commentDrawn)
// 	{
// 		const char* tag = m_ctx->m_Comments.Find(address);
// 		if(tag != nullptr)
// 		{
// 			std::string disAsm = (";") + string_cast<std::string>(tag);
// 			deviceContext.TextOut(metadataPosition, y, disAsm.c_str(), selected ? RGB(0x00, 0xF0, 0x00) : RGB(0x00, 0x80, 0x00));
// 			commentDrawn = true;
// 		}
// 	}
// }
