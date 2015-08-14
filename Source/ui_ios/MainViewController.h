#import <UIKit/UIKit.h>
#import "SqliteDatabase.h"
#import <SDWebImage/UIImageView+WebCache.h>

@interface MainViewController : UITableViewController

@property SqliteDatabase* database;
@property NSArray* images;

@end
