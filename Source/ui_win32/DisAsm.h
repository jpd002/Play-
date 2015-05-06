#pragma once

#include <boost/signals2.hpp>
#include "win32/CustomDrawn.h"
#include "win32/DeviceContext.h"
#include "win32/GdiObj.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CDisAsm : public Framework::Win32::CCustomDrawn, public boost::signals2::trackable
{
public:
	typedef boost::signals2::signal<void (uint32)> FindCallersRequestedEvent;

									CDisAsm(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CDisAsm();

	void							SetAddress(uint32);
	void							SetCenterAtAddress(uint32);
	void							SetSelectedAddress(uint32);

	FindCallersRequestedEvent		FindCallersRequested;

protected:
	struct RENDERMETRICS
	{
		int leftBarSize = 0;
		int xmargin = 0;
		int xtextStart = 0;
		int yspace = 0;
		int ymargin = 0;
		int fontSizeX = 0;
		int fontSizeY = 0;
	};

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
	long							OnKeyDown(WPARAM, LPARAM) override;
	long							OnCopy() override;

	virtual unsigned int			BuildContextMenu(HMENU);

	uint32							GetInstruction(uint32);

	CMIPS*							m_ctx;
	int32							m_instructionSize;
	RENDERMETRICS					m_renderMetrics;

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
	virtual unsigned int			GetMetadataPosition() const;
	virtual unsigned int			GetFuncBoundaryPosition() const;

	virtual void					DrawInstructionDetails(Framework::Win32::CDeviceContext&, uint32, int);
	void							DrawInstructionMetadata(Framework::Win32::CDeviceContext&, uint32, int, bool);

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
