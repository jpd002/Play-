#import "SettingsListSelectorViewController.h"

@implementation SettingsListSelectorViewController

- (void)viewDidAppear:(BOOL)animated
{
	UITableViewCell* cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:self.value inSection:0]];
	if(cell != nil)
	{
		cell.accessoryType = UITableViewCellAccessoryCheckmark;
	}
}

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
	self.value = [indexPath row];
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	[self performSegueWithIdentifier:@"returnToSettings" sender:self];
}

@end
