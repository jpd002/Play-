#ifndef _DISASM_H_
#define _DISASM_H_

#include <boost/signal.hpp>
#include "win32/CustomDrawn.h"
#include "../MIPS.h"

class CDisAsm : public Framework::Win32::CCustomDrawn, public boost::signals::trackable
{
public:
									CDisAsm(HWND, RECT*, CMIPS*);
									~CDisAsm();
	void							SetAddress(uint32);

protected:
	void							Paint(HDC);
	long							OnMouseWheel(short);
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnSetFocus();
	long							OnKillFocus();
	long							OnLeftButtonDown(int, int);
	long							OnLeftButtonDblClk(int, int);
	long							OnRightButtonUp(int, int);
	long							OnMouseMove(WPARAM, int, int);
	long							OnVScroll(unsigned int, unsigned int);
	long							OnKeyDown(unsigned int);
    long                            OnCopy();

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
	uint32							GetInstruction(uint32);
	unsigned int					GetLineCount();
	unsigned int					GetFontHeight();
	HFONT							GetFont();
	bool							IsAddressVisible(uint32);
    SelectionRangeType              GetSelectionRange();
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

	CMIPS*							m_pCtx;
	HBITMAP							m_nArrow;
	HBITMAP							m_nArrowMask;
	HBITMAP							m_nBPoint;
	HBITMAP							m_nBPointMask;
	uint32							m_nAddress;
	uint32							m_nSelected;
    uint32                          m_nSelectionEnd;
	int								m_nFontCX;
	bool							m_nFocus;

	uint32							m_nHistory[HISTORY_STACK_MAX];
	unsigned int					m_nHistoryPosition;
	unsigned int					m_nHistorySize;
};

#endif