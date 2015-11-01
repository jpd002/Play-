#import <UIKit/UIKit.h>
#include "PH_Generic.h"

@interface VirtualPadItem : NSObject

@property CGRect bounds;
@property(weak) UITouch* touch;
@property CPH_Generic* padHandler;
@property UIImage* image;

-(void)draw: (CGContextRef)context;
-(void)onPointerDown: (CGPoint)position;
-(void)onPointerMove: (CGPoint)position;
-(void)onPointerUp;

@end
