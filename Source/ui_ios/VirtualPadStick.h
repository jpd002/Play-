#import <UIKit/UIKit.h>
#import "VirtualPadItem.h"
#import "../ControllerInfo.h"

@interface VirtualPadStick : VirtualPadItem

@property PS2::CControllerInfo::BUTTON codeX;
@property PS2::CControllerInfo::BUTTON codeY;
@property CGPoint pressPosition;
@property CGPoint offset;

- (void)onPointerDown:(CGPoint)position;
- (void)onPointerUp;

@end
