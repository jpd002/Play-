#import <Cocoa/Cocoa.h>
#import "DebuggerWindow.h"

@interface CApplicationDelegate : NSObject 
{
	IBOutlet NSOpenGLView* m_openGlView;
	IBOutlet NSWindow* m_outputWindow;
	IBOutlet CDebuggerWindow* m_debuggerWindow;
}

-(void)applicationDidFinishLaunching: (NSNotification*)notification;
-(void)OnBootElf: (id)sender;
-(void)OnPauseResume: (id)sender;
-(void)OnSaveState: (id)sender;
-(void)OnLoadState: (id)sender;
-(void)BootFromElf: (NSString*)fileName;

@end
