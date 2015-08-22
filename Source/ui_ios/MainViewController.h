#import <UIKit/UIKit.h>
#import "SqliteDatabase.h"
#import <SDWebImage/UIImageView+WebCache.h>
#import "CollectionView.h"

@interface MainViewController : UITableViewController

@property SqliteDatabase* database;
@property CollectionView *mainView;

@end
