#import <UIKit/UIKit.h>

@interface SettingsViewController : UITableViewController
{
	IBOutlet UISwitch* showFpsSwitch;
	IBOutlet UISwitch* showVirtualPadSwitch;
	IBOutlet UISlider* virtualPadOpacitySlider;
	IBOutlet UISwitch* hideVirtualPadWhenControllerConnected;
    IBOutlet UISwitch* virtualPadHapticFeedbackSwitch;

	IBOutlet UILabel* gsHandlerName;
	IBOutlet UILabel* resolutionFactor;
	IBOutlet UISwitch* resizeOutputToWidescreen;
	IBOutlet UISwitch* forceBilinearFiltering;

	IBOutlet UISwitch* enableAudioOutput;

	IBOutlet UILabel* versionInfoLabel;
}

@property(copy, nonatomic) void (^completionHandler)();

- (IBAction)selectedGsHandler:(UIStoryboardSegue*)segue;
- (IBAction)selectedResolutionFactor:(UIStoryboardSegue*)segue;
- (IBAction)returnToParent;

@end
