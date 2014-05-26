#import "MainTabBarController.h"
#import "PsfLoader.h"
#import "SH_OpenAL.h"
#import "ObjCMemberFunctionPointer.h"
#import <boost/filesystem.hpp>
#import <boost/lexical_cast.hpp>
#import "string_cast.h"
#import <AVFoundation/AVAudioSession.h>
#import <MediaPlayer/MediaPlayer.h>
#import "NSStringUtils.h"

#define PLAY_STRING		@"Play"
#define PAUSE_STRING	@"Pause"

@implementation MainTabBarController

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

-(void)remoteControlReceivedWithEvent: (UIEvent*)receivedEvent
{
	if(receivedEvent.type == UIEventTypeRemoteControl)
	{
		switch(receivedEvent.subtype)
		{
		case UIEventSubtypeRemoteControlPlay:
			[self onPlayButtonPress];
			break;
		case UIEventSubtypeRemoteControlPause:
			[self onPlayButtonPress];
			break;
		case UIEventSubtypeRemoteControlTogglePlayPause:
			[self onPlayButtonPress];
			break;
		case UIEventSubtypeRemoteControlPreviousTrack:
			[self onPrevButtonPress];
			break;
		case UIEventSubtypeRemoteControlNextTrack:
			[self onNextButtonPress];
			break;
		default:
			NSLog(@"Received remote control event: %d", receivedEvent.subtype);
			break;
		}
	}
}

-(void)reset
{
	m_ready = false;
	m_playing = false;
	m_trackLength = 0;
	m_fadePosition = 0;
	m_frames = 0;
	m_volumeAdjust = 1.0f;
}

-(void)viewDidLoad
{
	m_playlist = nullptr;
	m_currentPlaylistItem = 0;
	m_repeatMode = PLAYLIST_REPEAT;
	
	m_virtualMachine = new CPsfVm();
	m_virtualMachine->OnNewFrame.connect(ObjCMemberFunctionPointer(self, sel_getUid("onNewFrame")));
	
	[NSTimer scheduledTimerWithTimeInterval: 0.20 target: self selector: @selector(onUpdateTrackTimeTimer) userInfo: nil repeats: YES];
	[NSTimer scheduledTimerWithTimeInterval: 0.05 target: self selector: @selector(onUpdateFadeTimer) userInfo: nil repeats: YES];
	
	[self reset];
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
		
	m_playlistViewController = (PlaylistViewController*)self.viewControllers[0];
	m_playlistViewController.delegate = self;
	
	m_fileInfoViewController = (FileInfoViewController*)self.viewControllers[1];
	m_fileInfoViewController.delegate = self;
	
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(onAudioSessionInterruption:) name: AVAudioSessionInterruptionNotification object: nil];
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(onAudioSessionRouteChanged:) name: AVAudioSessionRouteChangeNotification object: nil];
	NSError* setCategoryErr = nil;
	[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryPlayback error: &setCategoryErr];
	
	[[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
	[self becomeFirstResponder];
}

-(void)onPlayButtonPress
{
	if(!m_ready) return;
	if(m_playing)
	{
		[m_fileInfoViewController setPlayButtonText: PLAY_STRING];
		m_virtualMachine->Pause();
		m_playing = false;
		
		m_virtualMachine->SetSpuHandler(nullptr);
		
		NSError* activationErr = nil;
		BOOL success = [[AVAudioSession sharedInstance] setActive: NO error: &activationErr];
		assert(success == YES);
	}
	else
	{
		NSError* activationErr = nil;
		BOOL success = [[AVAudioSession sharedInstance] setActive: YES error: &activationErr];
		assert(success == YES);
		
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
	[self reset];
	[m_fileInfoViewController setPlayButtonText: PLAY_STRING];
	
	self.selectedIndex = 1;
	
	unsigned int archiveId = playlistItem.archiveId;
	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
		
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(
							*m_virtualMachine,
							playlistItem.path,
							(archiveId == 0) ? nullptr : m_playlist->GetArchive(archiveId),
							&tags);
		m_virtualMachine->SetReverbEnabled(true);
		m_virtualMachine->Resume();
		
		CPsfTags psfTags(tags);
		NSString* title = @"PsfPlayer";
		if(psfTags.HasTag("title"))
		{
			title = stringWithWchar(psfTags.DecodeTagValue(psfTags.GetRawTagValue("title").c_str()));
		}
		NSString* gameName = @"";
		if(psfTags.HasTag("game"))
		{
			gameName = stringWithWchar(psfTags.DecodeTagValue(psfTags.GetRawTagValue("game").c_str()));
		}
		
		try
		{
			m_volumeAdjust = boost::lexical_cast<float>(psfTags.GetTagValue("volume"));
			m_virtualMachine->SetVolumeAdjust(m_volumeAdjust);
		}
		catch(...)
		{
			
		}
	
		[m_fileInfoViewController setTrackTitle: title];
		[m_fileInfoViewController setTrackTime: @"00:00"];
		[m_fileInfoViewController setTags: psfTags];
		
		NSArray* keys = [NSArray arrayWithObjects: MPMediaItemPropertyAlbumTitle, MPMediaItemPropertyTitle, nil];
		NSArray* values = [NSArray arrayWithObjects: gameName, title, nil];
		NSDictionary* mediaInfo = [NSDictionary dictionaryWithObjects:values forKeys:keys];
		[MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = mediaInfo;
		
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
		[m_playlistViewController setPlayingItemIndex: itemIndex];
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
