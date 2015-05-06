#ifndef _THREADCALLSTACKVIEWWND_H_
#define _THREADCALLSTACKVIEWWND_H_

#include "win32/Dialog.h"
#include "win32/Button.h"
#include "win32/ListBox.h"
#include "win32/Layouts.h"
#include "../MIPS.h"
#include "../BiosDebugInfoProvider.h"

class CThreadCallStackViewWnd : public Framework::Win32::CDialog
{
public:
									CThreadCallStackViewWnd(HWND);
	virtual							~CThreadCallStackViewWnd();

	void							SetItems(CMIPS*, const CMIPSAnalysis::CallStackItemArray&, const BiosDebugModuleInfoArray&);

	bool							HasSelection() const;
	uint32							GetSelectedAddress() const;

protected:
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	void							ProcessSelection();

	Framework::Win32::CButton*		m_okButton;
	Framework::Win32::CButton*		m_cancelButton;
	Framework::Win32::CListBox*		m_callStackItemList;

	Framework::LayoutObjectPtr		m_layout;

	bool							m_hasSelection;
	uint32							m_selectedAddress;
};

#endif
