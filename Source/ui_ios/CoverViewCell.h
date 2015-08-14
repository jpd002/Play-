#import <UIKit/UIKit.h>

@interface CoverViewCell : UITableViewCell

@property (nonatomic, retain) IBOutlet UIImageView  *coverImage;
@property (nonatomic, retain) IBOutlet UILabel      *nameLabel;
@property (nonatomic, retain) IBOutlet UILabel      *overviewLabel;

@end
