#import "MainTabBarController.h"
#import "PsfLoader.h"
#import "SH_OpenAL.h"
#import "ObjCMemberFunctionPointer.h"
#import <boost/filesystem.hpp>
#import "string_cast.h"
#import <AVFoundation/AVAudioSession.h>

#define PLAY_STRING		@"Play"
#define PAUSE_STRING	@"Pause"

@implementation MainTabBarController

NSString* stringWithWchar(const std::wstring& input)
{
	NSString* result = [[NSString alloc] initWithBytes: input.data()
												length: input.length() * sizeof(wchar_t)
											  encoding: NSUTF32LittleEndianStringEncoding];
	[result autorelease];
	return result;
}

-(void)onAudioSessionInterruption: (NSNotification*)notification
{
	NSNumber* interruptionType = [notification.userInfo valueForKey: AVAudioSessionInterruptionTypeKey];
	if([interruptionType intValue] == AVAudioSessionInterruptionTypeBegan)
	{
		if(m_playing)
		{
			[self onPlayButtonPress];
		}
	}
}

-(void)onAudioSessionRouteChanged: (NSNotification*)notification
{
	AVAudioSessionRouteDescription* route = [notification.userInfo valueForKey: AVAudioSessionRouteChangePreviousRouteKey];
	if([route.outputs count] > 0)
	{
		AVAudioSessionPortDescription* port = [route.outputs objectAtIndex: 0];
		if([port.portType compare: AVAudioSessionPortHeadphones] == NSOrderedSame)
		{
			if(m_playing)
			{
				[self onPlayButtonPress];
			}
		}
	}
}

-(void)viewDidLoad
{
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(onAudioSessionInterruption:) name: AVAudioSessionInterruptionNotification object: nil];
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(onAudioSessionRouteChanged:) name: AVAudioSessionRouteChangeNotification object: nil];
	NSError* setCategoryErr = nil;
	[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryPlayback error: &setCategoryErr];
	
	m_virtualMachine = new CPsfVm();
	m_virtualMachine->OnNewFrame.connect(ObjCMemberFunctionPointer(self, sel_getUid("onNewFrame")));
	
	[NSTimer scheduledTimerWithTimeInterval: 1 target: self selector: @selector(onTimer) userInfo: nil repeats: YES];
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	
	m_frames = 0;
	m_playing = false;
	
	PlaylistViewController* playlistViewController = (PlaylistViewController*)self.viewControllers[0];
	playlistViewController.delegate = self;
	
	m_fileInfoViewController = (FileInfoViewController*)self.viewControllers[1];
	m_fileInfoViewController.delegate = self;
}

-(void)onPlayButtonPress
{
	if(m_playing)
	{
		[m_fileInfoViewController setPlayButtonText: PLAY_STRING];
		m_virtualMachine->Pause();
		m_playing = false;
		
		NSError* activationErr = nil;
		[[AVAudioSession sharedInstance] setActive: NO error: &activationErr];
	}
	else
	{
		NSError* activationErr = nil;
		[[AVAudioSession sharedInstance] setActive: YES error: &activationErr];
		m_virtualMachine->SetSpuHandler(&CSH_OpenAL::HandlerFactory);
		
		[m_fileInfoViewController setPlayButtonText: PAUSE_STRING];
		m_virtualMachine->Resume();
		m_playing = true;
	}
}

-(void)onPlaylistItemSelected: (const CPlaylist::ITEM&)playlistItem playlist: (CPlaylist*)playlist
{
	//Initialize UI
	m_frames = 0;
	m_playing = false;
	[m_fileInfoViewController setPlayButtonText: PLAY_STRING];
	
	self.selectedIndex = 1;
	
	unsigned int archiveId = playlistItem.archiveId;

	NSString* filePath = stringWithWchar(playlistItem.path);
	NSString* archivePath = nil;
	if(archiveId != 0)
	{
		archivePath = stringWithWchar(playlist->GetArchive(archiveId));
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
			NSString* titleString = stringWithWchar(title);
			[m_fileInfoViewController setTrackTitle: titleString];
		}
		[m_fileInfoViewController setTrackTime: @"00:00"];
		[m_fileInfoViewController setTags: psfTags];
		
		[self onPlayButtonPress];
	}
	catch(const std::exception& exception)
	{
		assert(0);
	}
}

-(void)onTimer
{
	const int fps = 60;
		
	int time = m_frames / fps;
	int seconds = time % 60;
	int minutes = time / 60;
	
	NSString* timeString = [NSString stringWithFormat: @"%0.2d:%0.2d", minutes, seconds];
	[m_fileInfoViewController setTrackTime: timeString];
}

-(void)onNewFrame
{
	m_frames++;
}

@end
