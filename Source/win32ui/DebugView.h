#ifndef _DEBUGVIEW_H_
#define _DEBUGVIEW_H_

#include "DisAsmWnd.h"
#include "MemoryViewMIPSWnd.h"
#include "RegViewWnd.h"
#include "CallStackWnd.h"

class CDebugView
{
public:
							CDebugView(HWND, CMIPS*, const char*);
	virtual					~CDebugView();
	CMIPS*					GetContext();
	CDisAsmWnd*				GetDisassemblyWindow();
	CMemoryViewMIPSWnd*		GetMemoryViewWindow();
	CRegViewWnd*			GetRegisterViewWindow();
	CCallStackWnd*			GetCallStackWindow();

	const char*				GetName() const;
	void					Hide();

protected:
	void					OnCallStackWndFunctionDblClick(uint32);

private:
	const char*				m_sName;

	CMIPS*					m_pCtx;
	CDisAsmWnd*				m_pDisAsmWnd;
	CMemoryViewMIPSWnd*		m_pMemoryViewWnd;
	CRegViewWnd*			m_pRegViewWnd;
	CCallStackWnd*			m_pCallStackWnd;
};

#endif
