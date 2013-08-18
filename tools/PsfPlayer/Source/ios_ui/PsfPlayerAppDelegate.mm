#import "PsfPlayerAppDelegate.h"
#import "PsfLoader.h"
#import "SH_OpenAL.h"
#import "ObjCMemberFunctionPointer.h"
#import <AVFoundation/AVAudioSession.h>
#import <boost/filesystem.hpp>
#import "string_cast.h"

#define PLAY_STRING		@"Play"
#define PAUSE_STRING	@"Pause"

@implementation PsfPlayerAppDelegate

@synthesize m_window;
@synthesize m_tabBarController;

NSString* stringWithWchar(const std::wstring& input)
{
	NSString* result = [[NSString alloc] initWithBytes: input.data()
												length: input.length() * sizeof(wchar_t)
											  encoding: NSUTF32LittleEndianStringEncoding];
	[result autorelease];
	return result;
}

-(void)applicationDidFinishLaunching: (UIApplication*)application 
{
	// Add the tab bar controller's current view as a subview of the window
	[m_window addSubview: m_tabBarController.view];
	m_window.rootViewController = m_tabBarController;
	
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

	m_virtualMachine->OnNewFrame.connect(ObjCMemberFunctionPointer(self, sel_getUid("onNewFrame")));
	
	[NSTimer scheduledTimerWithTimeInterval: 1 target: self selector: @selector(onTimer) userInfo: nil repeats: YES];
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	
	m_frames = 0;
	m_playing = false;
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
//	m_playlist->Read([selectedPlaylistPath fileSystemRepresentation]);
	
	{
		auto path = boost::filesystem::path([selectedPlaylistPath fileSystemRepresentation]);
		auto archive(CPsfArchive::CreateFromPath(path));
		
		unsigned int archiveId = m_playlist->InsertArchive(path.wstring().c_str());
		for(const auto& file : archive->GetFiles())
		{
			boost::filesystem::path filePath(file.name);
			std::string fileExtension = filePath.extension().string();
			if((fileExtension.length() != 0) && CPlaylist::IsLoadableExtension(fileExtension.c_str() + 1))
			{
				CPlaylist::ITEM newItem;
				newItem.path = filePath.wstring();
				newItem.title = string_cast<std::wstring>(newItem.path);
				newItem.length = 0;
				newItem.archiveId = archiveId;
				m_playlist->InsertItem(newItem);
			}
		}
	}
	
	[m_playlistViewController setPlaylist: m_playlist];
}

-(void)onTimer
{	
	const int fps = 60;
	
	if((m_prebufferOverlay != nil) && m_frames > fps)
	{
		[m_prebufferOverlay dismissWithClickedButtonIndex: 0 animated: YES];	
		[m_prebufferOverlay release];
		m_prebufferOverlay = nil;		
	}
	
	int time = m_frames / fps;
	int seconds = time % 60;
	int minutes = time / 60;
	
	NSString* timeString = [NSString stringWithFormat: @"%0.2d:%0.2d", minutes, seconds];
	
	m_currentTimeLabel.text = timeString;
}

-(void)onNewFrame
{
	m_frames++;
}

-(void)onPlaylistItemSelected
{	
	//Initialize UI
	m_frames = 0;
	m_playing = false;
	m_playButton.enabled = NO;
	[m_playButton setTitle: PLAY_STRING forState: UIControlStateNormal];

	m_tabBarController.selectedIndex = 1;
	[self showPrebufferOverlay];	
	
	CPlaylist::ITEM playlistItem;
	[m_playlistViewController selectedPlaylistItem: &playlistItem];
	unsigned int archiveId = playlistItem.archiveId;
	
	NSString* filePath = stringWithWchar(playlistItem.path);
	NSString* archivePath = nil;
	if(archiveId != 0)
	{
		archivePath = stringWithWchar(m_playlist->GetArchive(archiveId));
	}
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(
							*m_virtualMachine,
							[filePath fileSystemRepresentation],
							(archiveId == 0) ? NULL : [archivePath fileSystemRepresentation],
							&tags);
		m_virtualMachine->SetReverbEnabled(true);
		m_virtualMachine->Resume();
		
		CPsfTags psfTags(tags);
		if(psfTags.HasTag("title"))
		{
			std::wstring title = psfTags.DecodeTagValue(psfTags.GetRawTagValue("title").c_str());
			NSString* titleString = [[NSString alloc] initWithBytes: title.c_str() 
															 length: (title.length() * sizeof(wchar_t)) 
														   encoding: NSUTF32LittleEndianStringEncoding];
			m_trackTitleLabel.text = titleString;
			[titleString release];
		}
		[m_fileInfoViewController setTags: psfTags];
		
		m_playing = true;
		m_playButton.enabled = YES;
		[m_playButton setTitle: PAUSE_STRING forState: UIControlStateNormal];
	}
	catch(const std::exception& exception)
	{
		assert(0);
	}
}

@end
