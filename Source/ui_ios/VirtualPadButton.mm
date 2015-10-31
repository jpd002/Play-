#import "VirtualPadButton.h"

@implementation VirtualPadButton

-(void)draw: (CGContextRef)context
{
	[self.image drawInRect: self.bounds];
	if(self.pressed)
	{
		CGContextSaveGState(context);
		CGContextSetBlendMode(context, kCGBlendModePlusDarker);
		[self.image drawInRect: self.bounds];
		CGContextRestoreGState(context);
	}
	
	if([self.caption length] != 0)
	{
		NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
		paragraphStyle.alignment = NSTextAlignmentCenter;
		NSDictionary* attributes =
		@{
			NSParagraphStyleAttributeName: paragraphStyle,
			NSForegroundColorAttributeName: [UIColor whiteColor]
		};
	
		CGSize textSize = [self.caption sizeWithAttributes: attributes];
		CGRect textRect = CGRectOffset(self.bounds, 0, (self.bounds.size.height - textSize.height) / 2);
	
		[self.caption drawInRect: textRect withAttributes: attributes];
	}
}

-(void)onPointerDown: (CGPoint)position
{
	self.pressed = YES;
	self.padHandler->SetButtonState(self.code, true);
	[super onPointerDown: position];
}

-(void)onPointerUp
{
	self.pressed = NO;
	self.padHandler->SetButtonState(self.code, false);
	[super onPointerUp];
}

@end
