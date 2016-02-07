#import "VfsManagerViewController.h"

@implementation VfsManagerViewController

-(id)init
{
	if(self = [super initWithNibName: @"VfsManagerView" bundle: nil])
	{
	
	}
	return self;
}

-(void)awakeFromNib
{
	[bindingsTableView setDoubleAction: @selector(onTableViewDblClick:)];
}

-(void)viewWillDisappear
{
	[bindings save];
}

-(void)onTableViewDblClick: (id)sender
{
	int selectedIndex = [bindingsTableView clickedRow];
	if(selectedIndex == -1) return;
	VfsManagerBinding* binding = [bindings getBindingAt: selectedIndex];
	[binding requestModification];
}

@end
