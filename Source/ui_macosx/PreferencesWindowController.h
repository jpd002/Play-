#import <Cocoa/Cocoa.h>

@interface PreferencesWindowController : NSWindowController
{
	IBOutlet NSToolbar*		toolbar;
}

@property(retain) NSViewController* currentViewController;

+(PreferencesWindowController*)defaultController;
-(void)show;
-(IBAction)onToolBarButtonPressed: (id)sender;

@end
