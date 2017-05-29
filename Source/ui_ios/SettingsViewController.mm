#import "SettingsViewController.h"
#include "../AppConfig.h"
#include "PreferenceDefs.h"
#include "AppDef.h"
#include "../gs/GSH_OpenGL/GSH_OpenGL.h"

@implementation SettingsViewController

-(void)viewDidLoad
{
	[showFpsSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWFPS)];
	[showVirtualPadSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD)];

	[enableHighResMode setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_ENABLEHIGHRESMODE)];
	[forceBilinearFiltering setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES)];
	
	[enableAudioOutput setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT)];

	NSString* versionString = [NSString stringWithFormat: @"0.%02d - %s", APP_VERSION, __DATE__];
	versionInfoLabel.text = versionString;
}

-(void)viewDidDisappear: (BOOL)animated
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWFPS, showFpsSwitch.isOn);
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD, showVirtualPadSwitch.isOn);
	
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_ENABLEHIGHRESMODE, enableHighResMode.isOn);
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, forceBilinearFiltering.isOn);
	
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, enableAudioOutput.isOn);
	
	CAppConfig::GetInstance().Save();
}

@end
