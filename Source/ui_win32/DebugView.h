#pragma once

#include <string>
#include <functional>
#include "DisAsmWnd.h"
#include "MemoryViewMIPSWnd.h"
#include "RegViewWnd.h"
#include "CallStackWnd.h"
#include "../VirtualMachine.h"

class CDebugView : public boost::signals2::trackable
{
public:
	typedef std::function<void (void)> StepFunction;

								CDebugView(HWND, CVirtualMachine&, CMIPS*, const StepFunction&, CBiosDebugInfoProvider*, const char*, CDisAsmWnd::DISASM_TYPE = CDisAsmWnd::DISASM_STANDARD);
	virtual						~CDebugView();

	CMIPS*						GetContext();
	CDisAsmWnd*					GetDisassemblyWindow();
	CMemoryViewMIPSWnd*			GetMemoryViewWindow();
	CRegViewWnd*				GetRegisterViewWindow();
	CCallStackWnd*				GetCallStackWindow();

	void						Step();
	const char*					GetName() const;
	void						Hide();
	CBiosDebugInfoProvider*		GetBiosDebugInfoProvider() const;

protected:
	void						OnCallStackWndFunctionDblClick(uint32);

private:
	std::string 				m_name;

	CVirtualMachine&			m_virtualMachine;
	CMIPS*						m_ctx;
	CDisAsmWnd*					m_disAsmWnd;
	CMemoryViewMIPSWnd*			m_memoryViewWnd;
	CRegViewWnd*				m_regViewWnd;
	CCallStackWnd*				m_callStackWnd;
	StepFunction				m_stepFunction;
	CBiosDebugInfoProvider*		m_biosDebugInfoProvider;
};
