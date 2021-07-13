#import "../AppConfig.h"
#import "PreferenceDefs.h"
#import "VirtualPadButton.h"

@interface VirtualPadButton ()
@property(strong, nonatomic) UIImpactFeedbackGenerator* impactFeedback;
@property(strong, nonatomic) UISelectionFeedbackGenerator* selectionFeedback;
@end

@implementation VirtualPadButton

- (id)init
{
	self = [super init];
	if(self)
	{
		_impactFeedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
		_selectionFeedback = [[UISelectionFeedbackGenerator alloc] init];
	}
	return self;
}

- (void)draw:(CGContextRef)context
{
	[self.image drawInRect:self.bounds];
	if(self.pressed)
	{
		CGContextSaveGState(context);
		CGContextSetBlendMode(context, kCGBlendModePlusDarker);
		[self.image drawInRect:self.bounds];
		CGContextRestoreGState(context);
	}

	if([self.caption length] != 0)
	{
		NSMutableParagraphStyle* paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
		paragraphStyle.alignment = NSTextAlignmentCenter;
		NSDictionary* attributes =
		    @{
			    NSParagraphStyleAttributeName : paragraphStyle,
			    NSForegroundColorAttributeName : [UIColor whiteColor]
		    };

		CGSize textSize = [self.caption sizeWithAttributes:attributes];
		CGRect textRect = CGRectOffset(self.bounds, 0, (self.bounds.size.height - textSize.height) / 2);

		[self.caption drawInRect:textRect withAttributes:attributes];
	}
}

- (void)onPointerDown:(CGPoint)position
{
	self.pressed = YES;
	self.padHandler->SetButtonState(self.code, true);
	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_VIRTUALPAD_HAPTICFEEDBACK))
	{
		switch(self.code)
		{
		case PS2::CControllerInfo::BUTTON::START:
		case PS2::CControllerInfo::BUTTON::SELECT:
		case PS2::CControllerInfo::BUTTON::SQUARE:
		case PS2::CControllerInfo::BUTTON::TRIANGLE:
		case PS2::CControllerInfo::BUTTON::CROSS:
		case PS2::CControllerInfo::BUTTON::CIRCLE:
		case PS2::CControllerInfo::BUTTON::L1:
		case PS2::CControllerInfo::BUTTON::L2:
		case PS2::CControllerInfo::BUTTON::L3:
		case PS2::CControllerInfo::BUTTON::R1:
		case PS2::CControllerInfo::BUTTON::R2:
		case PS2::CControllerInfo::BUTTON::R3:
			[self.impactFeedback impactOccurred];
			break;

		default:
			[self.selectionFeedback selectionChanged];
			break;
		}
	}
	[super onPointerDown:position];
}

- (void)onPointerUp
{
	self.pressed = NO;
	self.padHandler->SetButtonState(self.code, false);
	[super onPointerUp];
}

@end
