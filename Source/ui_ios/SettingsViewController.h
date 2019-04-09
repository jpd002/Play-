#import <UIKit/UIKit.h>

@interface SettingsViewController : UITableViewController
{
	IBOutlet UISwitch* showFpsSwitch;
	IBOutlet UISwitch* showVirtualPadSwitch;

	IBOutlet UILabel* resolutionFactor;
	IBOutlet UISwitch* forceBilinearFiltering;

	IBOutlet UISwitch* enableAudioOutput;

	IBOutlet UILabel* versionInfoLabel;
}

- (IBAction)returnToSettings:(UIStoryboardSegue*)segue;

@end
