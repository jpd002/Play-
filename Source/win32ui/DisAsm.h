#pragma once

#include <boost/signal.hpp>
#include "win32/CustomDrawn.h"
#include "win32/DeviceContext.h"
#include "win32/GdiObj.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CDisAsm : public Framework::Win32::CCustomDrawn, public boost::signals::trackable
{
public:
									CDisAsm(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CDisAsm();

	void							SetAddress(uint32);
	void							SetCenterAtAddress(uint32);
	void							SetSelectedAddress(uint32);

protected:
	void							Paint(HDC) override;

	long							OnMouseWheel(int, int, short) override;
	long							OnSize(unsigned int, unsigned int, unsigned int) override;
	long							OnCommand(unsigned short, unsigned short, HWND) override;
	long							OnSetFocus() override;
	long							OnKillFocus() override;
	long							OnLeftButtonDown(int, int) override;
	long							OnLeftButtonDblClk(int, int) override;
	long							OnRightButtonUp(int, int) override;
	long							OnMouseMove(WPARAM, int, int) override;
	long							OnVScroll(unsigned int, unsigned int) override;
	long							OnKeyDown(unsigned int) override;
	long							OnCopy() override;

	uint32							GetInstruction(uint32);

	CMIPS*							m_ctx;
	int32							m_instructionSize;

private:
	enum
	{
		HISTORY_STACK_MAX = 20
	};

	typedef std::pair<uint32, uint32> SelectionRangeType;

	void							GotoAddress();
	void							GotoPC();
	void							GotoEA();
	void							EditComment();
	void							FindCallers();
	void							UpdatePosition(int);
	void							UpdateMouseSelection(unsigned int, unsigned int);
	void							ToggleBreakpoint(uint32);
	uint32							GetAddressAtPosition(unsigned int, unsigned int);
	unsigned int					GetLineCount();
	unsigned int					GetFontHeight();
	bool							IsAddressVisible(uint32);
	SelectionRangeType				GetSelectionRange();
	void							HistoryReset();
	void							HistorySave(uint32);
	void							HistoryGoBack();
	void							HistoryGoForward();
	uint32							HistoryGetPrevious();
	uint32							HistoryGetNext();
	bool							HistoryHasPrevious();
	bool							HistoryHasNext();
	void							OnMachineStateChange();
	void							OnRunningStateChange();
	
	virtual std::tstring			GetInstructionDetailsText(uint32);

	virtual void					DrawInstructionDetails(Framework::Win32::CDeviceContext&, uint32, int);
	void							DrawInstructionMetadata(Framework::Win32::CDeviceContext&, uint32, int);

	CVirtualMachine&				m_virtualMachine;
	HBITMAP							m_arrowBitmap;
	HBITMAP							m_arrowMaskBitmap;
	HBITMAP							m_breakpointBitmap;
	HBITMAP							m_breakpointMaskBitmap;
	uint32							m_address;
	uint32							m_selected;
	uint32							m_selectionEnd;
	bool							m_focus;
	Framework::Win32::CFont			m_font;

	uint32							m_history[HISTORY_STACK_MAX];
	unsigned int					m_historyPosition;
	unsigned int					m_historySize;
};
