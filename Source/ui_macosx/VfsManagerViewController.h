#import "VfsManagerBindings.h"
#import <Cocoa/Cocoa.h>

@interface VfsManagerViewController : NSViewController
{
	IBOutlet VfsManagerBindings* bindings;
	IBOutlet NSTableView* bindingsTableView;
}

@end
