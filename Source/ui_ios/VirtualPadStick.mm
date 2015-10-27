#import "VirtualPadStick.h"

@implementation VirtualPadStick

-(void)draw: (CGContextRef)context
{
//	if(self.pressed)
	if(false)
	{
		CGFloat color[] = { 0.75, 0.75, 0.75, 1.0 };
		CGContextSetFillColor(context, color);
	}
	else
	{
		CGFloat color[] = { 0.75, 0.75, 0.75, 0.5 };
		CGContextSetFillColor(context, color);
	}
	CGContextFillRect(context, self.bounds);
}

-(void)onPointerDown: (CGPoint)position
{
//	self.pressed = YES;
//	self.padHandler->SetButtonState(self.code, true);
	[super onPointerDown: position];
}

-(void)onPointerUp
{
//	self.pressed = NO;
//	self.padHandler->SetButtonState(self.code, false);
	[super onPointerUp];
}

@end
