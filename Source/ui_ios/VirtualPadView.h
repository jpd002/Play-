#import <UIKit/UIKit.h>
#import "VirtualPadItem.h"
#include "PH_Generic.h"

@interface VirtualPadView : UIView
{
	CPH_Generic* _padHandler;
}

@property NSArray<VirtualPadItem*>* items;
@property NSDictionary<NSString*, UIImage*>* itemImages;

-(VirtualPadView*)initWithFrame: (CGRect)frame padHandler: (CPH_Generic*) padHandler;
-(void)rebuildPadItems;

@end
