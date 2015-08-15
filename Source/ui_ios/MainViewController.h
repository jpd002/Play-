#import <UIKit/UIKit.h>
#import "SqliteDatabase.h"
#import <SDWebImage/UIImageView+WebCache.h>
#import "ViewOrientation.h"

@interface MainViewController : UITableViewController

@property SqliteDatabase* database;
@property ViewOrientation *mainView;

@end
