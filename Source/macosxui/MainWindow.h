#import <Cocoa/Cocoa.h>

@interface CMainWindow : NSWindow
{

}

-(bool)acceptsFirstResponder;
-(void)keyDown : (NSEvent*)event;

@end
