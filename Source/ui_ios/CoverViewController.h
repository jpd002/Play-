//
//  CoverViewController.h
//  Play
//
//  Created by Lounge Katt on 8/15/15.
//  Copyright (c) 2015 Jean-Philip Desjardins. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SqliteDatabase.h"
#import <SDWebImage/UIImageView+WebCache.h>
#import "CollectionView.h"

@interface CoverViewController : UICollectionViewController

@property SqliteDatabase* database;
@property CollectionView* coverView;

@end
