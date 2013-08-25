#import "PsfPlayerAppDelegate.h"
#import <AVFoundation/AVAudioSession.h>

@implementation PsfPlayerAppDelegate

@synthesize window;

-(void)applicationDidFinishLaunching: (UIApplication*)application 
{		
	NSError* setCategoryErr = nil;
	NSError* activationErr = nil;
	[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryPlayback error: &setCategoryErr];
	[[AVAudioSession sharedInstance] setActive: YES error: &activationErr];
}

/*
-(IBAction)onPauseButtonClick: (id)sender
{
	if(m_playing)
	{
		[m_playButton setTitle: PLAY_STRING forState: UIControlStateNormal];
		m_virtualMachine->Pause();
		m_playing = false;
	}
	else
	{
		[m_playButton setTitle: PAUSE_STRING forState: UIControlStateNormal];
		m_virtualMachine->Resume();
		m_playing = true;
	}
}
*/

@end
