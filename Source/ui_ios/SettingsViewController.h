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

	IBOutlet UISwitch* enableAltServerJIT;

	IBOutlet UILabel* versionInfoLabel;
}

@property bool allowGsHandlerSelection;
@property bool allowFullDeviceScan;
@property(copy, nonatomic) void (^completionHandler)(bool);

//Internal: This is set when the user presses the Full Device Scan button.
@property bool fullDeviceScanRequested;

- (IBAction)selectedGsHandler:(UIStoryboardSegue*)segue;
- (IBAction)selectedResolutionFactor:(UIStoryboardSegue*)segue;
- (IBAction)startFullDeviceScan;
- (IBAction)returnToParent;

@end
