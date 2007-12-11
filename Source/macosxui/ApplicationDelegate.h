#import <Cocoa/Cocoa.h>

@interface CApplicationDelegate : NSObject 
{
	IBOutlet NSOpenGLView* m_openGlView;
	IBOutlet NSWindow* m_outputWindow;
}

-(void)applicationDidFinishLaunching : (NSNotification*)notification;
-(void)BootElf : (id)sender;

@end
