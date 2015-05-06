#import <Cocoa/Cocoa.h>
#import "VfsManagerBindings.h"

@interface VfsManagerController : NSWindowController
{
	IBOutlet VfsManagerBindings*	m_bindings;
	IBOutlet NSTableView*			m_bindingsTableView;
}

+(VfsManagerController*)defaultController;
-(void)awakeFromNib;
-(void)showManager;
-(IBAction)OnTableViewDblClick: (id)sender;
-(void)OnOk: (id)sender;
-(void)OnCancel: (id)sender;
-(void)windowWillClose: (NSNotification*)notification;

@end
