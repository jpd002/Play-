//
//  ViewOrientation.h
//  Play
//
//  Created by Lounge Katt on 8/15/15.
//  Copyright (c) 2015 Jean-Philip Desjardins. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface CollectionView : NSObject
{
    BOOL isShowingLandscapeView;
    NSArray* diskImages;
}

@property (nonatomic) BOOL isShowingLandscapeView;
@property NSArray* diskImages;

+ (CollectionView*) getInstance;
- (NSArray*) buildCollection;

@end
