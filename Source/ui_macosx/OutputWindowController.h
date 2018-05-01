#import <Cocoa/Cocoa.h>

@protocol OutputWindowDelegate

- (void)outputWindowDidResize:(NSSize)size;

@end

@interface OutputWindowController : NSWindowController

@property(weak, nonatomic) IBOutlet NSOpenGLView* openGlView;
@property(weak, nonatomic) id<OutputWindowDelegate> delegate;

- (NSSize)contentSize;

@end
