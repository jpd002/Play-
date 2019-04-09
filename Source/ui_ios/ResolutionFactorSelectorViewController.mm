#import "ResolutionFactorSelectorViewController.h"

@implementation ResolutionFactorSelectorViewController

-(void)viewDidAppear: (BOOL)animated
{
	int index = log2(self.factor);
	UITableViewCell* cell = [self.tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: index inSection: 0]];
	if(cell != nil)
	{
		cell.accessoryType = UITableViewCellAccessoryCheckmark;
	}
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{
	self.factor = 1 << [indexPath row];
	[tableView deselectRowAtIndexPath: indexPath animated: YES];
	[self performSegueWithIdentifier: @"returnToSettings" sender: self];
}

@end
