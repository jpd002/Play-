#import <UIKit/UIKit.h>
#import "PlaylistViewController.h"
#import "FileInfoViewController.h"
#import "PsfVm.h"

enum REPEAT_MODE
{
	PLAYLIST_ONCE,
	PLAYLIST_REPEAT,
	PLAYLIST_SHUFFLE,
	TRACK_REPEAT
};

@interface MainTabBarController : UITabBarController <PlaylistViewControllerDelegate, FileInfoViewControllerDelegate>
{
	CPsfVm* m_virtualMachine;

	uint64 m_frames;
	bool m_playing;

	PlaylistViewController* m_playlistViewController;
	FileInfoViewController* m_fileInfoViewController;

	CPlaylist* m_playlist;
	unsigned int m_currentPlaylistItem;
	bool m_ready;

	REPEAT_MODE m_repeatMode;

	uint64 m_trackLength;
	uint64 m_fadePosition;
	float m_volumeAdjust;
}

@end
