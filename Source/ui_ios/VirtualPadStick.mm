#import "VirtualPadStick.h"

@implementation VirtualPadStick

-(void)draw: (CGContextRef)context
{
	CGFloat color[] = { 0.75, 0.75, 0.75, 0.5 };
	auto offsetBounds = CGRectOffset(self.bounds, self.offset.x, self.offset.y);
	CGContextSetFillColor(context, color);
	CGContextFillRect(context, offsetBounds);
}

-(void)onPointerDown: (CGPoint)position
{
	self.pressPosition = position;
	self.offset = CGPointMake(0, 0);
	[super onPointerDown: position];
}

-(void)onPointerMove: (CGPoint)position
{
	float radius = self.bounds.size.width;
	float offsetX = position.x - self.pressPosition.x;
	float offsetY = position.y - self.pressPosition.y;
	offsetX = std::min<float>(offsetX,  radius);
	offsetX = std::max<float>(offsetX, -radius);
	offsetY = std::min<float>(offsetY,  radius);
	offsetY = std::max<float>(offsetY, -radius);
	self.offset = CGPointMake(offsetX, offsetY);
	self.padHandler->SetAxisState(self.codeX, offsetX / radius);
	self.padHandler->SetAxisState(self.codeY, offsetY / radius);
	[super onPointerMove: position];
}

-(void)onPointerUp
{
	self.pressPosition = CGPointMake(0, 0);
	self.offset = CGPointMake(0, 0);
	self.padHandler->SetAxisState(self.codeX, 0);
	self.padHandler->SetAxisState(self.codeY, 0);
	[super onPointerUp];
}

@end
