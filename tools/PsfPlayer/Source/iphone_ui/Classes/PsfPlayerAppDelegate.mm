#import "PsfPlayerAppDelegate.h"
#import "PsfLoader.h"
#import "SH_OpenAL.h"
#import "ObjCMemberFunctionPointer.h"
#import <AVFoundation/AVAudioSession.h>

@implementation PsfPlayerAppDelegate

@synthesize m_window;
@synthesize m_tabBarController;

-(void)applicationDidFinishLaunching: (UIApplication*)application 
{
    // Add the tab bar controller's current view as a subview of the window
    [m_window addSubview: m_tabBarController.view];
	
	[m_playlistSelectViewController setSelectionHandler: self selector: @selector(onPlaylistSelected)];
	[m_playlistViewController setSelectionHandler: self selector: @selector(onPlaylistItemSelected)];
	
	NSError* setCategoryErr = nil;
	NSError* activationErr = nil;
	[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryPlayback error: &setCategoryErr];
	[[AVAudioSession sharedInstance] setActive: YES error: &activationErr];
	
	m_playlist = NULL;
	m_prebufferOverlay = nil;
	
	m_virtualMachine = new CPsfVm();
	m_virtualMachine->SetSpuHandler(&CSH_OpenAL::HandlerFactory);
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
}

-(void)dealloc 
{
    [m_tabBarController release];
    [m_window release];
    [super dealloc];
}

-(IBAction)onOpenPlaylist: (id)sender
{
	UINavigationController* navigationController = [[UINavigationController alloc] initWithRootViewController: m_playlistSelectViewController];
	[m_tabBarController presentModalViewController: navigationController animated: YES];
	[navigationController release];	
}

-(void)showPrebufferOverlay
{
	assert(m_prebufferOverlay == nil);
	
	int actionSheetWidth = 320;
	int actionSheetHeight = 175;
	int activityIndicatorWidth = 30;
	int activityIndicatorHeight = 30;
	
	CGRect activityIndicatorRect = CGRectMake(
											  (actionSheetWidth - activityIndicatorWidth) / 2, 
											  (actionSheetHeight - activityIndicatorHeight) / 2 - (activityIndicatorHeight / 2), 
											  activityIndicatorWidth, 
											  activityIndicatorHeight);
	
	UIActivityIndicatorView* progressView = [[UIActivityIndicatorView alloc] initWithFrame: activityIndicatorRect];
	progressView.activityIndicatorViewStyle = UIActivityIndicatorViewStyleWhiteLarge;
	
	m_prebufferOverlay = [[UIActionSheet alloc] initWithTitle: @"Pre-buffering"
													 delegate: self
											cancelButtonTitle: nil
									   destructiveButtonTitle: nil
											otherButtonTitles: nil];
	
	UIViewController* viewController = m_tabBarController.selectedViewController;
	
	[m_prebufferOverlay addSubview: progressView];
	[m_prebufferOverlay showInView: viewController.view];
	[m_prebufferOverlay setBounds: CGRectMake(0, 0, 320, 175)];
	
	[progressView startAnimating];
	[progressView release];	
}

-(void)onPlaylistSelected
{
	NSString* selectedPlaylistPath = [m_playlistSelectViewController selectedPlaylistPath];	
	if(m_playlist != NULL)
	{
		delete m_playlist;
		m_playlist = NULL;
	}
	m_playlist = new CPlaylist();
	m_playlist->Read([selectedPlaylistPath fileSystemRepresentation]);
	[m_playlistViewController setPlaylist: m_playlist];
}

-(void)onPlayStartedImpl
{
	[m_prebufferOverlay dismissWithClickedButtonIndex: 0 animated: YES];	
	[m_prebufferOverlay release];
	m_prebufferOverlay = nil;
}

-(void)onPlayStarted
{
	[self performSelectorOnMainThread: @selector(onPlayStartedImpl) withObject: nil waitUntilDone: NO];
}

-(void)onPlaylistItemSelected
{
	m_tabBarController.selectedIndex = 1;
	[self showPrebufferOverlay];
	
	NSString* filePath = [m_playlistViewController selectedPlaylistItemPath];
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(*m_virtualMachine, [filePath fileSystemRepresentation], &tags);
//		m_virtualMachine->SetReverbEnabled(false);
		m_virtualMachine->OnPlayStarted.connect(ObjCMemberFunctionPointer(self, sel_getUid("onPlayStarted")));
		m_virtualMachine->Resume();
	}
	catch(const std::exception& exception)
	{
		assert(0);
	}
}

@end
