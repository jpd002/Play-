#import <UIKit/UIKit.h>
#import "VirtualPadItem.h"
#import "../ControllerInfo.h"

@interface VirtualPadButton : VirtualPadItem

@property BOOL pressed;
@property PS2::CControllerInfo::BUTTON code;

-(void)onPointerDown: (CGPoint)position;
-(void)onPointerUp;

@end
