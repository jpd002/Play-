#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "TApplication.h"
#include "DebuggerWnd.h"
#include "PsfVm.h"

class CApplication : public TApplication
{
public:
                            CApplication();
    virtual                 ~CApplication();
        
protected:
    virtual Boolean         HandleCommand(const HICommandExtended& inCommand);
	
private:
	CDebuggerWnd*			m_debuggerWnd;
	CPsfVm					m_virtualMachine;
};

#endif
