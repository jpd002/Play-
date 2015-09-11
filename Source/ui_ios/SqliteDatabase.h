#import <Foundation/Foundation.h>
#import <sqlite3.h>

@interface SqliteDatabase : NSObject
{
    sqlite3 *_db;
}

- (void)loadDatabase;

- (NSDictionary *)getDiskInfo:(NSString *)serial;

@end
