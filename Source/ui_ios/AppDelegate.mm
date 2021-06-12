#import "AppDelegate.h"
#import "EmulatorViewController.h"
#include "../gs/GSH_OpenGL/GSH_OpenGL.h"
#include "DebuggerSimulator.h"
#import "AltKit-Swift.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOption
{
	[[ALTServerManager sharedManager] startDiscovering];

	[[ALTServerManager sharedManager] autoconnectWithCompletionHandler:^(ALTServerConnection* connection, NSError* error) {
	  if(error)
	  {
		  return NSLog(@"Could not auto-connect to server. %@", error);
	  }

	  [connection enableUnsignedCodeExecutionWithCompletionHandler:^(BOOL success, NSError* error) {
		if(success)
		{
			NSLog(@"Successfully enabled JIT compilation!");
			[[ALTServerManager sharedManager] stopDiscovering];
		}
		else
		{
			NSLog(@"Could not enable JIT compilation. %@", error);
		}

		[connection disconnect];
	  }];
	}];

	[EmulatorViewController registerPreferences];
	CGSH_OpenGL::RegisterPreferences();
	return YES;
}

- (void)applicationWillResignActive:(UIApplication*)application
{
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
}

- (void)applicationWillTerminate:(UIApplication*)application
{
	StopSimulateDebugger();
}

@end
