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
	m_playlist = nullptr;
	m_currentPlaylistItem = 0;
	m_ready = false;
	m_repeatMode = PLAYLIST_REPEAT;
	m_trackLength = 0;
	m_fadePosition = 0;
	m_volumeAdjust = 1.0f;
	
	m_virtualMachine = new CPsfVm();
	m_virtualMachine->OnNewFrame.connect(ObjCMemberFunctionPointer(self, sel_getUid("onNewFrame")));
	
	[NSTimer scheduledTimerWithTimeInterval: 0.20 target: self selector: @selector(onUpdateTrackTimeTimer) userInfo: nil repeats: YES];
	[NSTimer scheduledTimerWithTimeInterval: 0.05 target: self selector: @selector(onUpdateFadeTimer) userInfo: nil repeats: YES];
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	
	m_frames = 0;
	m_playing = false;
	
	PlaylistViewController* playlistViewController = (PlaylistViewController*)self.viewControllers[0];
	playlistViewController.delegate = self;
	
	m_fileInfoViewController = (FileInfoViewController*)self.viewControllers[1];
	m_fileInfoViewController.delegate = self;
	
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(onAudioSessionInterruption:) name: AVAudioSessionInterruptionNotification object: nil];
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(onAudioSessionRouteChanged:) name: AVAudioSessionRouteChangeNotification object: nil];
	NSError* setCategoryErr = nil;
	[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryPlayback error: &setCategoryErr];
}

-(void)onPlayButtonPress
{
	if(!m_ready) return;
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

-(void)onPrevButtonPress
{
	if(m_playlist->GetItemCount() == 0) return;
	if(m_repeatMode == PLAYLIST_SHUFFLE)
	{
		//TODO
		assert(0);
	}
	else
	{
		if(m_currentPlaylistItem != 0)
		{
			m_currentPlaylistItem--;
		}
		else
		{
			m_currentPlaylistItem = m_playlist->GetItemCount() - 1;
		}
	}
	[self onPlaylistItemSelected: m_currentPlaylistItem];
}

-(void)onNextButtonPress
{
	if(m_playlist->GetItemCount() == 0) return;
	unsigned int itemCount = m_playlist->GetItemCount();
	if(m_repeatMode == PLAYLIST_SHUFFLE)
	{
		//TODO
		assert(0);
	}
	else
	{
		if((m_currentPlaylistItem + 1) < itemCount)
		{
			m_currentPlaylistItem++;
		}
		else
		{
			m_currentPlaylistItem = 0;
		}
	}
	[self onPlaylistItemSelected: m_currentPlaylistItem];
}

-(void)onPlaylistSelected: (CPlaylist*)playlist
{
	m_playlist = playlist;
	[self onPlaylistItemSelected: 0];
}

-(void)onPlaylistItemSelected: (unsigned int)itemIndex
{
	const auto& playlistItem(m_playlist->GetItem(itemIndex));
	
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
			NSString* titleString = stringWithWchar(title);
			[m_fileInfoViewController setTrackTitle: titleString];
		}
		[m_fileInfoViewController setTrackTime: @"00:00"];
		[m_fileInfoViewController setTags: psfTags];
		
		if(m_repeatMode != TRACK_REPEAT)
		{
			double fade = 10.f;
			if(psfTags.HasTag("length"))
			{
				double length = CPsfTags::ConvertTimeString(psfTags.GetTagValue("length").c_str());
				m_trackLength = static_cast<uint64>(length * 60.0f);
			}
			else
			{
				m_trackLength = 60.f * 60.f;	//1 minute default length
			}
			if(psfTags.HasTag("fade"))
			{
				fade = CPsfTags::ConvertTimeString(psfTags.GetTagValue("fade").c_str());
			}
			m_fadePosition = m_trackLength;
			m_trackLength += static_cast<uint64>(fade * 60.f);
		}

		m_ready = true;
		m_currentPlaylistItem = itemIndex;
		[self onPlayButtonPress];
	}
	catch(const std::exception& exception)
	{
		assert(0);
	}
}

-(void)onUpdateTrackTimeTimer
{
	const int fps = 60;
		
	int time = m_frames / fps;
	int seconds = time % 60;
	int minutes = time / 60;
	
	NSString* timeString = [NSString stringWithFormat: @"%0.2d:%0.2d", minutes, seconds];
	[m_fileInfoViewController setTrackTime: timeString];
}

-(void)onUpdateFadeTimer
{
	if(m_trackLength != 0)
	{
		if(m_frames >= m_trackLength)
		{
			[self onNextButtonPress];
		}
		else if(m_frames >= m_fadePosition)
		{
			float currentRatio = static_cast<float>(m_frames - m_fadePosition) / static_cast<float>(m_trackLength - m_fadePosition);
			float currentVolume = (1.0f - currentRatio) * m_volumeAdjust;
			m_virtualMachine->SetVolumeAdjust(currentVolume);
		}
	}
}

-(void)onNewFrame
{
	m_frames++;
}

@end
