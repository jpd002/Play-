#import "SettingsViewController.h"
#include "../AppConfig.h"
#include "PreferenceDefs.h"
#include "AppDef.h"
#include "GSH_OpenGL.h"

@implementation SettingsViewController

-(void)viewDidLoad
{
	[showFpsSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWFPS)];
	[showVirtualPadSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD)];

	[forceBilinearFiltering setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES)];
	
	NSString* versionString = [NSString stringWithFormat: @"0.%0.2d - %s", APP_VERSION, __DATE__];
	versionInfoLabel.text = versionString;
}

-(void)viewDidDisappear: (BOOL)animated
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWFPS, showFpsSwitch.isOn);
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD, showVirtualPadSwitch.isOn);
	
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, forceBilinearFiltering.isOn);
	
	CAppConfig::GetInstance().Save();
}

@end
