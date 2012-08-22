#import <Cocoa/Cocoa.h>

@interface OutputWindow : NSWindow
{

}

-(bool)acceptsFirstResponder;
-(void)keyDown : (NSEvent*)event;

@end
