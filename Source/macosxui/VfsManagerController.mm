#import "VfsManagerController.h"

static CVfsManagerController* g_sharedInstance = nil;

@implementation CVfsManagerController

+(CVfsManagerController*)defaultController
{
	if(g_sharedInstance == nil)
	{
		g_sharedInstance = [[self alloc] initWithWindowNibName: @"VfsManager"];
	}
	return g_sharedInstance;
}

-(void)showManager
{
	[[self window] makeKeyAndOrderFront: nil];
}

@end
