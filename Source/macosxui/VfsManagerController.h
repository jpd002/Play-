#import <Cocoa/Cocoa.h>
#import "VfsManagerBindings.h"

@interface CVfsManagerController : NSWindowController
{
	IBOutlet CVfsManagerBindings*	m_bindings;
	IBOutlet NSTableView*			m_bindingsTableView;
}

+(CVfsManagerController*)defaultController;
-(void)awakeFromNib;
-(void)showManager;
-(IBAction)OnTableViewDblClick: (id)sender;
-(void)OnOk: (id)sender;
-(void)OnCancel: (id)sender;
-(void)windowWillClose: (NSNotification*)notification;

@end
