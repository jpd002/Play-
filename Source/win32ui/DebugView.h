#ifndef _DEBUGVIEW_H_
#define _DEBUGVIEW_H_

#include <string>
#include "DisAsmWnd.h"
#include "MemoryViewMIPSWnd.h"
#include "RegViewWnd.h"
#include "CallStackWnd.h"
#include "../VirtualMachine.h"

class CDebugView : public boost::signals::trackable
{
public:
							CDebugView(HWND, CVirtualMachine&, CMIPS*, const char*);
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
    std::string 			m_name;

	CMIPS*					m_pCtx;
	CDisAsmWnd*				m_pDisAsmWnd;
	CMemoryViewMIPSWnd*		m_pMemoryViewWnd;
	CRegViewWnd*			m_pRegViewWnd;
	CCallStackWnd*			m_pCallStackWnd;
};

#endif
