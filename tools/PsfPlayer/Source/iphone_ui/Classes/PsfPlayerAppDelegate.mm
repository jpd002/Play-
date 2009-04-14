//
//  PsfPlayerAppDelegate.m
//  PsfPlayer
//
//  Created by Jean-Philip Desjardins on 05/04/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "PsfPlayerAppDelegate.h"
#include "PsfLoader.h"
#include "SH_OpenAL.h"

@implementation PsfPlayerAppDelegate

@synthesize window;
@synthesize tabBarController;


- (void)applicationDidFinishLaunching:(UIApplication *)application {
    
    // Add the tab bar controller's current view as a subview of the window
    [window addSubview:tabBarController.view];
	
	m_virtualMachine = new CPsfVm();
	m_virtualMachine->SetSpuHandler(&CSH_OpenAL::HandlerFactory);
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	
//	NSString* filePath = [[NSBundle mainBundle] pathForResource:@"101 - Dearly Beloved" ofType:@"psf2"];
	NSString* filePath = [[NSBundle mainBundle] pathForResource:@"220 - Hollow Bastion" ofType:@"psf2"];
//	NSString* filePath = [[NSBundle mainBundle] pathForResource:@"vp-114" ofType:@"minipsf"];
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(*m_virtualMachine, [filePath fileSystemRepresentation], &tags);
		m_virtualMachine->SetReverbEnabled(false);
		m_virtualMachine->Resume();
	}
	catch(const std::exception& exception)
	{
		
	}
}


/*
// Optional UITabBarControllerDelegate method
- (void)tabBarController:(UITabBarController *)tabBarController didSelectViewController:(UIViewController *)viewController {
}
*/

/*
// Optional UITabBarControllerDelegate method
- (void)tabBarController:(UITabBarController *)tabBarController didEndCustomizingViewControllers:(NSArray *)viewControllers changed:(BOOL)changed {
}
*/


- (void)dealloc 
{
    [tabBarController release];
    [window release];
    [super dealloc];
}

@end

