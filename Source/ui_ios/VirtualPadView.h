#include "PH_Generic.h"
#import "VirtualPadItem.h"
#import <UIKit/UIKit.h>

@interface VirtualPadView : UIView
{
	CPH_Generic* _padHandler;
}

@property NSArray<VirtualPadItem*>* items;
@property NSDictionary<NSString*, UIImage*>* itemImages;

- (VirtualPadView*)initWithFrame:(CGRect)frame padHandler:(CPH_Generic*)padHandler;
- (void)rebuildPadItems;

@end
