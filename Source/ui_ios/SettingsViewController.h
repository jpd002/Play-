#import <UIKit/UIKit.h>

@interface SettingsViewController : UITableViewController
{
	IBOutlet UISwitch* showFpsSwitch;
	IBOutlet UISwitch* showVirtualPadSwitch;

	IBOutlet UISwitch* enableHighResMode;
	IBOutlet UISwitch* forceBilinearFiltering;

	IBOutlet UISwitch* enableAudioOutput;

	IBOutlet UILabel* versionInfoLabel;
}

@end
