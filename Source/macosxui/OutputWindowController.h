#import <Cocoa/Cocoa.h>

@protocol OutputWindowDelegate

-(void)outputWindowDidResize: (NSSize)size;

@end

@interface OutputWindowController : NSWindowController
{
	NSOpenGLView*					_openGlView;
	id<OutputWindowDelegate>		_delegate;
}

@property(assign, nonatomic) IBOutlet NSOpenGLView* openGlView;

-(void)setDelegate: (id<OutputWindowDelegate>)delegate;
-(NSSize)contentSize;

@end
