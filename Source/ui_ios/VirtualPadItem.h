#import <UIKit/UIKit.h>
#include "PH_iOS.h"

@interface VirtualPadItem : NSObject

@property CGRect bounds;
@property(weak) UITouch* touch;
@property CPH_iOS* padHandler;
@property UIImage* image;

-(void)draw: (CGContextRef)context;
-(void)onPointerDown: (CGPoint)position;
-(void)onPointerMove: (CGPoint)position;
-(void)onPointerUp;

@end
