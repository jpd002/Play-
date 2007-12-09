#import <Cocoa/Cocoa.h>

@interface CApplicationDelegate : NSObject 
{
	IBOutlet NSOpenGLView* m_openGlView;
}

-(void)applicationDidFinishLaunching : (NSNotification*)notification;
-(void)BootElf : (id)sender;

@end
