//
//  CoverViewController.h
//  Play
//
//  Created by Lounge Katt on 8/15/15.
//  Copyright (c) 2015 Jean-Philip Desjardins. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "../ui_shared/BootablesDbClient.h"

typedef std::vector<BootablesDb::Bootable> BootableArray;

@interface CoverViewController : UICollectionViewController
{
	BootableArray* _bootables;
}

- (IBAction)onExit:(id)handler;

@end
