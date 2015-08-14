#import "SqliteDatabase.h"

@implementation SqliteDatabase

//NSString *const dbVersion = @"1";

- (id)init
{
    self = [super init];
    
    if (self) {
        [self loadDatabase];
    }
    
    return self;
}

- (void)loadDatabase
{
//    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
//    
//    if (![[defaults objectForKey:@"DbVersion"] isEqualToString:dbVersion]) {
//        if ([[NSFileManager defaultManager] fileExistsAtPath:[self dbPath]]) {
//            [[NSFileManager defaultManager] removeItemAtPath:[self dbPath] error:nil];
//        }
//        [defaults setObject:dbVersion forKey:@"DbVersion"];
//        [defaults synchronize];
//    }
//    NSString *filePath = [[NSBundle mainBundle] pathForResource:@"games.db" ofType:nil];
//    NSError *error;
//    if (![[NSFileManager defaultManager] fileExistsAtPath:[self dbPath]]) {
//        if(![[NSFileManager defaultManager] copyItemAtPath:filePath toPath:[self dbPath] error:&error]) {
//            NSLog(@"Error copying the database: %@", [error description]);
//            
//        }
//    }
    if (sqlite3_open([[self dbPath] UTF8String], &_db) != SQLITE_OK) {
        NSLog(@"Cannot open the database.");
    }
}

- (NSString *)dbPath
{
//    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
//    NSString *documentDirectory = [paths objectAtIndex:0];
//    return [documentDirectory stringByAppendingPathComponent:@"games.db"];
    return [[NSBundle mainBundle] pathForResource:@"games.db" ofType:nil];
}

- (NSDictionary *)getDiskInfo:(NSString *)diskId
{
    NSString *gameID = @"";
    NSString *title = @"";
    NSString *overview = @"";
    NSString *serial = diskId;
    NSString *boxart = @"404";
        
    const char* selectSQL = "SELECT * FROM games WHERE Serial = ?";
    sqlite3_stmt *statement;
    
    if (sqlite3_prepare_v2(_db, selectSQL, -1, &statement, nil) == SQLITE_OK) {
        sqlite3_bind_text(statement, 1, [diskId UTF8String], (int) diskId.length, NULL);
        
        if (sqlite3_step(statement) != SQLITE_DONE) {
            gameID = [NSString stringWithUTF8String:(char *)sqlite3_column_text(statement, 1)];
            title = [NSString stringWithUTF8String:(char *)sqlite3_column_text(statement, 2)];
            overview = [NSString stringWithUTF8String:(char *)sqlite3_column_text(statement, 3)];
            serial = [NSString stringWithUTF8String:(char *)sqlite3_column_text(statement, 4)];
            boxart = [NSString stringWithUTF8String:(char *)sqlite3_column_text(statement, 5)];
        }
    }
    
    sqlite3_finalize(statement);
    
    return @{
         @"gameID": gameID,
         @"title": title,
         @"overview": overview,
         @"serial": serial,
         @"boxart": boxart
    };
}

@end
