#ifndef _DISASM_H_
#define _DISASM_H_

#include "win32/CustomDrawn.h"
#include "MIPS.h"
#include "EventHandler.h"

class CDisAsm : public Framework::CCustomDrawn
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
	long							OnLeftButtonDown(unsigned int, unsigned int);
	long							OnLeftButtonDblClk(unsigned int, unsigned int);
	long							OnRightButtonUp(unsigned int, unsigned int);
	long							OnMouseMove(WPARAM, unsigned int, unsigned int);
	long							OnVScroll(unsigned int, unsigned int);
	long							OnKeyDown(unsigned int);

private:
	enum
	{
		HISTORY_STACK_MAX = 20
	};

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
	void							HistoryReset();
	void							HistorySave(uint32);
	void							HistoryGoBack();
	void							HistoryGoForward();
	uint32							HistoryGetPrevious();
	uint32							HistoryGetNext();
	bool							HistoryHasPrevious();
	bool							HistoryHasNext();
	void							OnMachineStateChange(int);
	void							OnRunningStateChange(int);


	CMIPS*							m_pCtx;
	HBITMAP							m_nArrow;
	HBITMAP							m_nArrowMask;
	HBITMAP							m_nBPoint;
	HBITMAP							m_nBPointMask;
	uint32							m_nAddress;
	uint32							m_nSelected;
	int								m_nFontCX;
	bool							m_nFocus;

	uint32							m_nHistory[HISTORY_STACK_MAX];
	unsigned int					m_nHistoryPosition;
	unsigned int					m_nHistorySize;

	Framework::CEventHandler<int>*	m_pOnMachineStateChangeHandler;
	Framework::CEventHandler<int>*	m_pOnRunningStateChangeHandler;
};

#endif