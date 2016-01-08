#import <UIKit/UIKit.h>

@interface SettingsViewController : UITableViewController
{
	IBOutlet UISwitch*    showFpsSwitch;
	IBOutlet UISwitch*    showVirtualPadSwitch;
	
	IBOutlet UILabel*    versionInfoLabel;
}

@end
