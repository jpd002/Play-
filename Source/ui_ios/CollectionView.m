//
//  ViewOrientation.m
//  Play
//
//  Created by Lounge Katt on 8/15/15.
//  Copyright (c) 2015 Jean-Philip Desjardins. All rights reserved.
//

#import "CollectionView.h"
#import "IosUtils.h"
#import "DiskUtils.h"

@implementation CollectionView

@synthesize diskImages;

static CollectionView *instance = nil;

+ (CollectionView *) getInstance
{
    @synchronized(self)
    {
        if (instance == nil)
        {
            instance = [CollectionView new];
        }
    }
    return instance;
}

- (NSArray *) buildCollection
{
    NSString* path = [@"~" stringByExpandingTildeInPath];
    NSFileManager* localFileManager = [[NSFileManager alloc] init];
    
    NSDirectoryEnumerator* dirEnum = [localFileManager enumeratorAtPath: path];
    
    NSMutableArray* images = [[NSMutableArray alloc] init];
    
    NSString* file = nil;
    while(file = [dirEnum nextObject])
    {
        std::string diskId;
        if(
           IosUtils::IsLoadableExecutableFileName(file) ||
           (IosUtils::IsLoadableDiskImageFileName(file) &&
            DiskUtils::TryGetDiskId([[NSString stringWithFormat:@"%@/%@", path, file] UTF8String], &diskId))
           )
        {
            NSMutableDictionary *disk = [[NSMutableDictionary alloc] init];
            [disk setValue:[NSString stringWithUTF8String:diskId.c_str()] forKey:@"serial"];
            [disk setValue:file forKey:@"file"];
            [images addObject: disk];
            NSLog(@"%@: %s", file, diskId.c_str());
        }
    }
    
    self.diskImages = [images sortedArrayUsingComparator:^NSComparisonResult(id a, id b) {
        NSString *first = [[a objectForKey:@"file"] lastPathComponent];
        NSString *second = [[b objectForKey:@"file"] lastPathComponent];
        return [first caseInsensitiveCompare:second];
    }];
    
    return self.diskImages;
}

@end
