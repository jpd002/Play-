#import <Cocoa/Cocoa.h>
#import "VfsManagerBindings.h"

@interface VfsManagerViewController : NSViewController
{
	IBOutlet VfsManagerBindings*    bindings;
	IBOutlet NSTableView*           bindingsTableView;
}

@end
