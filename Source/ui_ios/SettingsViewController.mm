#import "SettingsViewController.h"
#import "ResolutionFactorSelectorViewController.h"
#include "../AppConfig.h"
#include "PreferenceDefs.h"
#include "AppDef.h"
#include "../gs/GSH_OpenGL/GSH_OpenGL.h"

@implementation SettingsViewController

-(void)updateResolutionFactorLabel
{
	int factor = CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSH_OPENGL_RESOLUTION_FACTOR);
	[resolutionFactor setText: [NSString stringWithFormat: @"%dx", factor]];
}

-(void)viewDidLoad
{
	[showFpsSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWFPS)];
	[showVirtualPadSwitch setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD)];

	[self updateResolutionFactorLabel];
	[forceBilinearFiltering setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES)];
	
	[enableAudioOutput setOn: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT)];

	NSString* versionString = [NSString stringWithFormat: @"%s - %s", PLAY_VERSION, __DATE__];
	versionInfoLabel.text = versionString;
}

-(void)viewDidDisappear: (BOOL)animated
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWFPS, showFpsSwitch.isOn);
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD, showVirtualPadSwitch.isOn);
	
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, forceBilinearFiltering.isOn);
	
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, enableAudioOutput.isOn);
	
	CAppConfig::GetInstance().Save();
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{
	[tableView deselectRowAtIndexPath: indexPath animated: YES];
}

-(void)prepareForSegue: (UIStoryboardSegue*)segue sender: (id)sender
{
	if([segue.destinationViewController isKindOfClass: [ResolutionFactorSelectorViewController class]])
	{
		ResolutionFactorSelectorViewController* selector = (ResolutionFactorSelectorViewController*)segue.destinationViewController;
		selector.factor = CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSH_OPENGL_RESOLUTION_FACTOR);
	}
}

-(IBAction)returnToSettings: (UIStoryboardSegue*)segue
{
	if([segue.sourceViewController isKindOfClass: [ResolutionFactorSelectorViewController class]])
	{
		ResolutionFactorSelectorViewController* selector = (ResolutionFactorSelectorViewController*)segue.sourceViewController;
		CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSH_OPENGL_RESOLUTION_FACTOR, selector.factor);
		[self updateResolutionFactorLabel];
	}
}

@end
