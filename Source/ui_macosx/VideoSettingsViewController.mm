#import "VideoSettingsViewController.h"
#include "../gs/GSH_OpenGL/GSH_OpenGL.h"
#include "../AppConfig.h"

@implementation VideoSettingsViewController

-(id)init
{
	if(self = [super initWithNibName: @"VideoSettingsView" bundle: nil])
	{
	
	}
	return self;
}

-(void)viewWillAppear
{
	[enableHighResModeCheckBox setState: CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_ENABLEHIGHRESMODE) ? NSOnState : NSOffState];
	[forceBilinearFilteringCheckBox setState: CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES) ? NSOnState : NSOffState];
}

-(void)viewWillDisappear
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_ENABLEHIGHRESMODE, enableHighResModeCheckBox.state == NSOnState);
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, forceBilinearFilteringCheckBox.state == NSOnState);
}

@end
