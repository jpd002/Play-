//
//  ViewOrientation.m
//  Play
//
//  Created by Lounge Katt on 8/15/15.
//  Copyright (c) 2015 Jean-Philip Desjardins. All rights reserved.
//

#import "ViewOrientation.h"

@implementation ViewOrientation

@synthesize isShowingLandscapeView;

static ViewOrientation *instance = nil;

+(ViewOrientation *)getInstance
{
    @synchronized(self)
    {
        if(instance==nil)
        {
            instance= [ViewOrientation new];
        }
    }
    return instance;
}

@end
