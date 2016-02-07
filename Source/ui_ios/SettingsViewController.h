#import <UIKit/UIKit.h>

@interface SettingsViewController : UITableViewController
{
	IBOutlet UISwitch*    showFpsSwitch;
	IBOutlet UISwitch*    showVirtualPadSwitch;
	
	IBOutlet UISwitch*    enableHighResMode;
	IBOutlet UISwitch*    forceBilinearFiltering;
	
	IBOutlet UILabel*    versionInfoLabel;
}

@end
