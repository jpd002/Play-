#import <UIKit/UIKit.h>
#import "VirtualPadItem.h"
#include "PH_iOS.h"

@interface VirtualPadView : UIView
{
	CPH_iOS* _padHandler;
}

@property NSArray<VirtualPadItem*>* items;

-(VirtualPadView*)initWithFrame: (CGRect)frame padHandler: (CPH_iOS*) padHandler;
-(void)rebuildPadItems;

@end
