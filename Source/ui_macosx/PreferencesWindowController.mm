#import <Foundation/Foundation.h>
#import "PreferencesWindowController.h"
#import "VideoSettingsViewController.h"
#import "AudioSettingsViewController.h"
#import "VfsManagerViewController.h"
#include "../AppConfig.h"

@implementation PreferencesWindowController

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
	[self.window center];
	[NSApp runModalForWindow: self.window];
	CAppConfig::GetInstance().Save();
}

-(void)windowWillClose: (NSNotification*)notification
{
	[NSApp stopModal];
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
		self.currentViewController = [[VideoSettingsViewController alloc] init];
		break;
	case 1:
		self.currentViewController = [[AudioSettingsViewController alloc] init];
		break;
	case 2:
		self.currentViewController = [[VfsManagerViewController alloc] init];
		break;
	default:
		assert(false);
		break;
	}
	NSView* contentView = self.currentViewController.view;
	self.window.contentView = nil;
	auto currentFrame = self.window.frame;
	auto currentTop = currentFrame.origin.y + currentFrame.size.height;
	auto windowFrame = [self.window frameRectForContentRect: contentView.frame];
	auto newY = currentTop - windowFrame.size.height;
	windowFrame = NSMakeRect(currentFrame.origin.x, newY, windowFrame.size.width, windowFrame.size.height);
	[self.window setFrame: windowFrame display: YES animate: YES];
	self.window.contentView = contentView;
}

@end
