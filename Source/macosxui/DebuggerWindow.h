#import <Cocoa/Cocoa.h>
#import "DisAsmView.h"
#import "RegView.h"

@interface CDebuggerWindow : NSWindow
{
	IBOutlet CDisAsmView*	m_disAsmView;
	IBOutlet CRegView*		m_regView;
}

-(void)Initialize;

@end
