#import "AudioSettingsViewController.h"
#include "../AppConfig.h"
#include "PreferenceDefs.h"

@implementation AudioSettingsViewController

-(id)init
{
	if(self = [super initWithNibName: @"AudioSettingsView" bundle: nil])
	{
	
	}
	return self;
}

-(void)viewWillAppear
{
	[enableAudioOutputCheckBox setState: CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT) ? NSOnState : NSOffState];
}

-(void)viewWillDisappear
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, enableAudioOutputCheckBox.state == NSOnState);
}

@end
