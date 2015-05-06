#import "OutputWindowController.h"

@interface OutputWindowController ()

@end

@implementation OutputWindowController

@synthesize openGlView = _openGlView;

-(void)setDelegate: (id)delegate
{
	_delegate = delegate;
}

-(NSApplicationPresentationOptions)window: (NSWindow*)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions
{
	return (NSApplicationPresentationFullScreen | NSApplicationPresentationHideDock | NSApplicationPresentationAutoHideMenuBar);
}

-(NSSize)window: (NSWindow*)window willUseFullScreenContentSize: (NSSize)proposedSize
{
	return proposedSize;
}

-(void)windowDidResize: (NSNotification*)notification
{
	[_delegate outputWindowDidResize: [self contentSize]];
}

-(NSSize)contentSize
{
	NSView* contentView = self.window.contentView;
	NSRect contentFrame = contentView.frame;
	return contentFrame.size;
}

@end
