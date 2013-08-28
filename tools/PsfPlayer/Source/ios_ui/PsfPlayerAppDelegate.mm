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

@end
