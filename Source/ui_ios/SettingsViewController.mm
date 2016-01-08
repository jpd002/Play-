#import "SettingsViewController.h"
#include "../AppConfig.h"
#include "PreferenceDefs.h"

@implementation SettingsViewController

-(void)viewDidLoad
{
	[showFpsSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWFPS)];
	[showVirtualPadSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD)];
}

-(void)viewDidDisappear: (BOOL)animated
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWFPS, showFpsSwitch.isOn);
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD, showVirtualPadSwitch.isOn);
	
	CAppConfig::GetInstance().Save();
}

@end
