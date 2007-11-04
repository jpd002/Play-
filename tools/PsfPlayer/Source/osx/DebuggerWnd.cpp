#include "DebuggerWnd.h"
#include <string>

//using namespace Framework;
using namespace std;

const HIViewID g_disAsmViewId = { 'disa', 100 };
const HIViewID g_registerViewId = { 'regs', 101 };

static HIViewRef FindView(WindowRef windowRef, const HIViewID& viewId)
{
	HIViewRef viewRef;
	HIViewFindByID(HIViewGetRoot(windowRef), viewId, &viewRef);	
	return viewRef;
}

CDebuggerWnd::CDebuggerWnd(CMIPS& cpu) :
TWindow(CFSTR("DebuggerWnd")),
m_cpu(cpu),
m_disAsmView(FindView(GetWindowRef(), g_disAsmViewId), cpu),
m_registerView(FindView(GetWindowRef(), g_registerViewId), cpu)
{
    // Position new windows in a staggered arrangement on the main screen
    RepositionWindow(GetWindowRef(), NULL, kWindowCenterOnMainScreen);

	EventTypeSpec keyEventSpec[] = { 
		kEventClassKeyboard, kEventRawKeyRepeat,
		kEventClassKeyboard, kEventRawKeyDown,
	};
	
	InstallWindowEventHandler(
		GetWindowRef(),
		NewEventHandlerUPP(KeyEventHandlerStub),
		GetEventTypeCount(keyEventSpec),
		keyEventSpec,
		this,
		NULL);
		
    // The window was created hidden, so show it
	Show();
}

CDebuggerWnd::~CDebuggerWnd()
{

}

OSStatus CDebuggerWnd::KeyEventHandler(EventHandlerCallRef handler, EventRef eventRef)
{
	char pressedKey;
	
	GetEventParameter(
		eventRef,
		kEventParamKeyMacCharCodes,
		typeChar,
		NULL,
		sizeof(char),
		NULL,
		&pressedKey);
	
	switch(pressedKey)
	{
		case 's':
			m_cpu.Step();
			m_disAsmView.EnsureVisible(m_cpu.m_State.nPC);
			m_disAsmView.SendRedraw();
			m_registerView.SendRedraw();
			break;
	}
	return 0;
}

OSStatus CDebuggerWnd::KeyEventHandlerStub(EventHandlerCallRef handler, EventRef eventRef, void* param)
{
	return reinterpret_cast<CDebuggerWnd*>(param)->KeyEventHandler(handler, eventRef);
}

Boolean CDebuggerWnd::HandleCommand(const HICommandExtended& inCommand)
{
    switch (inCommand.commandID)
    {
	default:
		return false;
    }
}
