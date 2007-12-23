#import "DebuggerWindow.h"
#include "Globals.h"

@implementation CDebuggerWindow

-(void)Initialize
{
	[m_disAsmView setContext:&g_virtualMachine->m_EE];
	[m_regView setContext:&g_virtualMachine->m_EE];
}

@end
