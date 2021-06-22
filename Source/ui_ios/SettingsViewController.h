#import <UIKit/UIKit.h>

@interface SettingsViewController : UITableViewController
{
	IBOutlet UISwitch* showFpsSwitch;
	IBOutlet UISwitch* showVirtualPadSwitch;

	IBOutlet UILabel* resolutionFactor;
	IBOutlet UISwitch* resizeOutputToWidescreen;
	IBOutlet UISwitch* forceBilinearFiltering;

	IBOutlet UISwitch* enableAudioOutput;

	IBOutlet UILabel* versionInfoLabel;
}

@property(copy, nonatomic) void (^completionHandler)();

- (IBAction)returnToSettings:(UIStoryboardSegue*)segue;
- (IBAction)returnToParent;

@end
