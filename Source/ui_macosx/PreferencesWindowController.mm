#import <Foundation/Foundation.h>
#import "PreferencesWindowController.h"
#import "VideoSettingsViewController.h"
#include "../AppConfig.h"

@implementation PreferencesWindowController

@synthesize currentViewController;

static PreferencesWindowController* g_sharedInstance = nil;

+(PreferencesWindowController*)defaultController
{
	if(g_sharedInstance == nil)
	{
		g_sharedInstance = [[self alloc] initWithWindowNibName: @"PreferencesWindow"];
	}
	return g_sharedInstance;
}

-(void)show
{
	[NSApp runModalForWindow: [self window]];
}

-(void)windowWillClose: (NSNotification*)notification
{
	CAppConfig::GetInstance().Save();
	[NSApp stopModal];
	[self release];
	g_sharedInstance = nil;
}

-(void)awakeFromNib
{
	NSToolbarItem* item = [toolbar.items objectAtIndex: 0];
	toolbar.selectedItemIdentifier = item.itemIdentifier;
	[self onToolBarButtonPressed: item];
}

-(IBAction)onToolBarButtonPressed: (id)sender
{
	NSToolbarItem* item = (NSToolbarItem*)sender;
	switch(item.tag)
	{
	case 0:
		currentViewController = [[VideoSettingsViewController alloc] init];
		break;
	default:
		assert(false);
		break;
	}
	NSView* contentView = currentViewController.view;
	self.window.contentView = nil;
	auto currentOrigin = self.window.frame.origin;
	auto windowFrame = [self.window frameRectForContentRect: contentView.frame];
	windowFrame = NSMakeRect(currentOrigin.x, currentOrigin.y, windowFrame.size.width, windowFrame.size.height);
	[self.window setFrame: windowFrame display: YES animate: YES];
	self.window.contentView = contentView;
}

@end
