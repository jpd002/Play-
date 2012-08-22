#import <Cocoa/Cocoa.h>
#import "OutputWindowController.h"

@interface ApplicationDelegate : NSObject
{
	OutputWindowController*		outputWindowController;
	IBOutlet NSMenuItem*		pauseResumeMenuItem;
}

-(void)applicationDidFinishLaunching: (NSNotification*)notification;
-(IBAction)bootElfMenuSelected: (id)sender;
-(IBAction)bootCdrom0MenuSelected: (id)sender;
-(IBAction)bootDiskImageSelected: (id)sender;
-(IBAction)pauseResumeMenuSelected: (id)sender;
-(IBAction)saveStateMenuSelected: (id)sender;
-(IBAction)loadStateMenuSelected: (id)sender;
-(IBAction)vfsManagerMenuSelected: (id)sender;
-(void)bootFromElf: (NSString*)fileName;
-(void)bootFromCdrom0;

@end
