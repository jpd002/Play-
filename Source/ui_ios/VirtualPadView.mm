#import "VirtualPadView.h"
#import "VirtualPadButton.h"
#import "VirtualPadStick.h"
#include "../VirtualPad.h"

@implementation VirtualPadView

-(id)initWithFrame: (CGRect)frame padHandler: (CPH_iOS*)padHandler
{
	self = [super initWithFrame: frame];
	if(self)
	{
		self.opaque = NO;
		self.multipleTouchEnabled = YES;
		_padHandler = padHandler;
		[self rebuildPadItems];
	}
	return self;
}

-(void)rebuildPadItems
{
	auto frame = self.frame;
	auto padItems = CVirtualPad::GetItems(frame.size.width, frame.size.height);
	NSMutableArray<VirtualPadItem*>* items = [[NSMutableArray alloc] init];
	for(const auto& padItem : padItems)
	{
		auto itemWidth = padItem.x2 - padItem.x1;
		auto itemHeight = padItem.y2 - padItem.y1;
		VirtualPadItem* item = nil;
		if(padItem.isAnalog)
		{
			auto stick = [[VirtualPadStick alloc] init];
			stick.codeX = padItem.code0;
			stick.codeY = padItem.code1;
			item = stick;
		}
		else
		{
			auto button = [[VirtualPadButton alloc] init];
			button.code = padItem.code0;
			item = button;
		}
		item.padHandler = _padHandler;
		item.bounds = CGRectMake(padItem.x1, padItem.y1, itemWidth, itemHeight);
		[items addObject: item];
	}
	self.items = items;
}

-(void)drawRect: (CGRect)rect
{
	[super drawRect: rect];
	
	auto context = UIGraphicsGetCurrentContext();
	
	for(VirtualPadItem* item in self.items)
	{
		[item draw: context];
	}
}

-(void)touchesBegan: (NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
	for(UITouch* touch in touches)
	{
		auto touchPos = [touch locationInView: self];
		for(VirtualPadItem* item in self.items)
		{
			if(CGRectContainsPoint(item.bounds, touchPos))
			{
				item.touch = touch;
				[item onPointerDown: touchPos];
				[self setNeedsDisplay];
				break;
			}
		}
	}
}

-(void)touchesMoved: (NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
	for(UITouch* touch in touches)
	{
		auto touchPos = [touch locationInView: self];
		for(VirtualPadItem* item in self.items)
		{
			if(item.touch == touch)
			{
				[item onPointerMove: touchPos];
				[self setNeedsDisplay];
				break;
			}
		}
	}
}

-(void)endOrCancelTouches: (NSSet<UITouch*>*)touches
{
	for(UITouch* touch in touches)
	{
		for(VirtualPadItem* item in self.items)
		{
			if(item.touch == touch)
			{
				item.touch = nil;
				[item onPointerUp];
				[self setNeedsDisplay];
				break;
			}
		}
	}
}

-(void)touchesEnded: (NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
	[self endOrCancelTouches: touches];
}

-(void)touchesCancelled: (NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
	[self endOrCancelTouches: touches];
}

@end
