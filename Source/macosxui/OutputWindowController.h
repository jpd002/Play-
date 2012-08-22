#import <Cocoa/Cocoa.h>

@interface OutputWindowController : NSWindowController
{
	NSOpenGLView* _openGlView;
}

@property(assign, nonatomic) IBOutlet NSOpenGLView* openGlView;

@end
