#import "../ControllerInfo.h"
#import "VirtualPadItem.h"
#import <UIKit/UIKit.h>

@interface VirtualPadButton : VirtualPadItem

@property BOOL pressed;
@property PS2::CControllerInfo::BUTTON code;
@property NSString* caption;

- (void)onPointerDown:(CGPoint)position;
- (void)onPointerUp;

@end
