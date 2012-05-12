#ifndef _DEBUGVIEW_H_
#define _DEBUGVIEW_H_

#include <string>
#include <functional>
#include "DisAsmWnd.h"
#include "MemoryViewMIPSWnd.h"
#include "RegViewWnd.h"
#include "CallStackWnd.h"
#include "../VirtualMachine.h"

class CDebugView : public boost::signals::trackable
{
public:
	typedef std::function<void (void)> StepFunction;

							CDebugView(HWND, CVirtualMachine&, CMIPS*, const StepFunction&, const char*);
	virtual					~CDebugView();
	CMIPS*					GetContext();
	CDisAsmWnd*				GetDisassemblyWindow();
	CMemoryViewMIPSWnd*		GetMemoryViewWindow();
	CRegViewWnd*			GetRegisterViewWindow();
	CCallStackWnd*			GetCallStackWindow();

	void					Step();
	const char*				GetName() const;
	void					Hide();

protected:
	void					OnCallStackWndFunctionDblClick(uint32);

private:
	std::string 			m_name;

	CVirtualMachine&		m_virtualMachine;
	CMIPS*					m_pCtx;
	CDisAsmWnd*				m_pDisAsmWnd;
	CMemoryViewMIPSWnd*		m_pMemoryViewWnd;
	CRegViewWnd*			m_pRegViewWnd;
	CCallStackWnd*			m_pCallStackWnd;
	StepFunction			m_stepFunction;
};

#endif
