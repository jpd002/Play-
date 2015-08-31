#import "CoverViewCell.h"

@implementation CoverViewCell

- (id)initWithFrame:(CGRect)frame
{
	
	self = [super initWithFrame:frame];
	
	if (self) {
		// Initialization code
		NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"coverCell" owner:self options:nil];
		
		if ([arrayOfViews count] < 1) {
			return nil;
		}
		
		if (![[arrayOfViews objectAtIndex:0] isKindOfClass:[UICollectionViewCell class]]) {
			return nil;
		}
		
		self = [arrayOfViews objectAtIndex:0];
		
	}
	
	return self;
	
}

- (void)awakeFromNib
{
    // Initialization code
}

@end
