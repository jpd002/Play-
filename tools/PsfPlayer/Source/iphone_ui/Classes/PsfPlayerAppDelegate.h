//
//  PsfPlayerAppDelegate.h
//  PsfPlayer
//
//  Created by Jean-Philip Desjardins on 05/04/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "PsfVm.h"

@interface PsfPlayerAppDelegate : NSObject <UIApplicationDelegate, UITabBarControllerDelegate> 
{
	CPsfVm*					m_virtualMachine;
    UIWindow*				window;
    UITabBarController*		tabBarController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UITabBarController *tabBarController;

@end
