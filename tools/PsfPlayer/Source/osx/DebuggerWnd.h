#ifndef _DEBUGGERWND_H_
#define _DEBUGGERWND_H_

#include "TWindow.h"
#include "DisAsmView.h"
#include "RegisterView.h"
#include "MIPS.h"

class CDebuggerWnd : public TWindow
{
public:
							CDebuggerWnd(CMIPS&);
	virtual					~CDebuggerWnd();

protected:
	virtual Boolean			HandleCommand(const HICommandExtended& inCommand);
	OSStatus				KeyEventHandler(EventHandlerCallRef, EventRef);
	static OSStatus			KeyEventHandlerStub(EventHandlerCallRef, EventRef, void*);
	
	CDisAsmView				m_disAsmView;
	CRegisterView			m_registerView;
	CMIPS&					m_cpu;
};

#endif
